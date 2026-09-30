import sys
import struct
import pefile
import unicorn
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn import x86_const as x86

def extract_aiv(aiv_filename):
    with open('Stronghold Crusader.exe', 'rb') as f:
        exe_bytes = f.read()

    pe = pefile.PE(data=exe_bytes)
    image_base = pe.OPTIONAL_HEADER.ImageBase

    # Init Unicorn
    mu = Uc(UC_ARCH_X86, UC_MODE_32)

    # Map PE sections
    for sec in pe.sections:
        vaddr = image_base + sec.VirtualAddress
        vsize = (sec.Misc_VirtualSize + 0xfff) & ~0xfff
        mu.mem_map(vaddr, vsize)
        sec_data = sec.get_data()
        mu.mem_write(vaddr, sec_data)

    # Map stack and heap
    STACK_BASE = 0x20000000
    STACK_SIZE = 0x100000
    HEAP_BASE  = 0x30000000
    HEAP_SIZE  = 0x200000
    mu.mem_map(STACK_BASE, STACK_SIZE)
    mu.mem_map(HEAP_BASE, HEAP_SIZE)

    # IAT hooks for GlobalAlloc/Lock/Unlock:
    # 0x59e060: GlobalAlloc
    # 0x59e05c: GlobalLock
    # 0x59e058: GlobalUnlock
    mu.mem_write(0x59e060, struct.pack('<I', 0x10001000))
    mu.mem_write(0x59e05c, struct.pack('<I', 0x10002000))
    mu.mem_write(0x59e058, struct.pack('<I', 0x10003000))
    mu.mem_write(0x59e054, struct.pack('<I', 0x10003000)) # GlobalFree

    # Map fake API handlers
    mu.mem_map(0x10000000, 0x10000)
    # GlobalAlloc(flags, size) -> returns handle (HEAP_BASE)
    # ret 8
    mu.mem_write(0x10001000, bytes([0xb8]) + struct.pack('<I', HEAP_BASE) + bytes([0xc2, 0x08, 0x00]))
    # GlobalLock(handle) -> returns pointer (HEAP_BASE)
    # ret 4
    mu.mem_write(0x10002000, bytes([0xb8]) + struct.pack('<I', HEAP_BASE) + bytes([0xc2, 0x04, 0x00]))
    # GlobalUnlock(handle) -> returns 0
    # ret 4
    mu.mem_write(0x10003000, bytes([0x31, 0xc0, 0xc2, 0x04, 0x00]))

    # Read AIV file
    with open(aiv_filename, 'rb') as f:
        aiv_bytes = f.read()

    hdr = aiv_bytes[4:4+0x7f0]
    num_sections = struct.unpack('<I', hdr[4:8])[0]
    uncomp_sizes = struct.unpack('<100I', hdr[0x1c:0x1c+400])
    comp_sizes   = struct.unpack('<100I', hdr[0x1ac:0x1ac+400])
    tags         = struct.unpack('<100I', hdr[0x33c:0x33c+400])
    flags        = struct.unpack('<100I', hdr[0x4cc:0x4cc+400])
    offsets      = struct.unpack('<100I', hdr[0x65c:0x65c+400])

    payload = aiv_bytes[2036:]

    def decompress_section(tag_id):
        idx = tags.index(tag_id)
        u_size = uncomp_sizes[idx]
        c_size = comp_sizes[idx]
        off    = offsets[idx]
        is_comp = flags[idx]

        comp_data = payload[off : off + c_size]

        if not is_comp:
            return comp_data[:u_size]

        # The compressed stream has an 8-byte header: (uncomp_size, comp_size)
        stream_data = comp_data[8:]
        stream_len = len(stream_data)

        # Allocate memory for src and dst in Unicorn
        SRC_ADDR = 0x40000000
        DST_ADDR = 0x41000000
        mu.mem_map(SRC_ADDR, 0x100000)
        mu.mem_map(DST_ADDR, 0x100000)

        mu.mem_write(SRC_ADDR, stream_data)

        # Setup call to 0x4725a0:
        # __thiscall 0x4725a0(ecx=helper, p1, p2, p3, p4, p5)
        # In 0x473cca:
        # push ecx (uncompressed size)
        # push edx (compressed size)
        # push ecx (src buffer)
        # push edi (dst buffer)
        # push eax (helper struct)
        # call 0x4725a0
        HELPER_ADDR = 0x50000000
        mu.mem_map(HELPER_ADDR, 0x10000)

        esp = STACK_BASE + STACK_SIZE - 0x1000
        mu.reg_write(x86.UC_X86_REG_ESP, esp)
        mu.reg_write(x86.UC_X86_REG_ECX, HELPER_ADDR)

        # Push arguments (cdecl / stdcall on stack):
        # [esp+0x14]: uncompressed size
        # [esp+0x10]: compressed size
        # [esp+0x0c]: src
        # [esp+0x08]: dst
        # [esp+0x04]: helper
        # [esp]: return address (0x10004000)
        mu.mem_write(0x10004000, bytes([0xf4])) # HLT instruction

        stack_args = struct.pack('<IIIIII', 0x10004000, HELPER_ADDR, DST_ADDR, SRC_ADDR, stream_len, u_size)
        mu.mem_write(esp, stack_args)

        # Run 0x4725a0 until 0x10004000
        try:
            mu.emu_start(0x4725a0, 0x10004000, timeout=10000000)
        except unicorn.UcError as e:
            pc = mu.reg_read(x86.UC_X86_REG_EIP)
            print(f'Emulation error at PC={hex(pc)}: {e}')

        ret_val = mu.reg_read(x86.UC_X86_REG_EAX)
        print(f'0x4725a0 returned: {ret_val}')

        decompressed = mu.mem_read(DST_ADDR, u_size)
        print('First 16 decompressed bytes:', bytes(decompressed[:16]).hex(' '))


        mu.mem_unmap(SRC_ADDR, 0x100000)
        mu.mem_unmap(DST_ADDR, 0x100000)
        mu.mem_unmap(HELPER_ADDR, 0x10000)

        return bytes(decompressed)

    print(f'Extracting {aiv_filename}...')
    grid_bytes = decompress_section(2007)
    step_bytes = decompress_section(2008)

    print(f'Decompressed Tag 2007 (Building Grid): {len(grid_bytes)} bytes')
    print(f'Decompressed Tag 2008 (Step Grid):     {len(step_bytes)} bytes')

    # Parse 100x100 grid
    buildings = []
    keep_x, keep_y = -1, -1

    for y in range(100):
        for x in range(100):
            idx = y * 100 + x
            b_type = struct.unpack('<H', grid_bytes[idx*2 : (idx+1)*2])[0]
            step   = struct.unpack('<I', step_bytes[idx*4 : (idx+1)*4])[0]
            if b_type != 0:
                if b_type == 38 or b_type == 0x26: # Keep
                    keep_x = x
                    keep_y = y
                buildings.append((x, y, b_type, step))

    print(f'Total buildings: {len(buildings)}')
    print(f'Keep location: ({keep_x}, {keep_y})')

    # Save to file
    out_name = aiv_filename.replace('.aiv', '_parsed.txt')
    with open(out_name, 'w') as out_f:
        out_f.write(f'# Castle: {aiv_filename}\n')
        out_f.write(f'# Keep: {keep_x} {keep_y}\n')
        out_f.write(f'# Total: {len(buildings)}\n')
        out_f.write('# dx dy b_type step\n')
        for x, y, b_type, step in buildings:
            out_f.write(f'{x - keep_x} {y - keep_y} {b_type} {step}\n')

    print(f'Saved to {out_name}!')

if __name__ == '__main__':
    extract_aiv('aiv/snake1.aiv')
