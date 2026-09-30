#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include "blueprint.h"
#include "overlay.h"

typedef HRESULT (WINAPI *PFN_SHGetFolderPathA)(HWND, int, HANDLE, DWORD, LPSTR);
typedef HRESULT (WINAPI *PFN_SHGetFolderPathW)(HWND, int, HANDLE, DWORD, LPWSTR);

static HMODULE g_hRealShFolder = NULL;
static PFN_SHGetFolderPathA g_pfnSHGetFolderPathA = NULL;
static PFN_SHGetFolderPathW g_pfnSHGetFolderPathW = NULL;
static HINSTANCE g_hInstance = NULL;

static void EnsureRealDllLoaded() {
    if (g_hRealShFolder) return;
    char sysPath[MAX_PATH];
    GetSystemDirectoryA(sysPath, MAX_PATH);
    strcat_s(sysPath, "\\shfolder.dll");
    g_hRealShFolder = LoadLibraryA(sysPath);
    if (!g_hRealShFolder) {
        GetSystemDirectoryA(sysPath, MAX_PATH);
        strcat_s(sysPath, "\\shell32.dll");
        g_hRealShFolder = LoadLibraryA(sysPath);
    }
    if (g_hRealShFolder) {
        g_pfnSHGetFolderPathA = (PFN_SHGetFolderPathA)GetProcAddress(g_hRealShFolder, "SHGetFolderPathA");
        g_pfnSHGetFolderPathW = (PFN_SHGetFolderPathW)GetProcAddress(g_hRealShFolder, "SHGetFolderPathW");
    }
}

extern "C" HRESULT WINAPI SHGetFolderPathA(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPSTR pszPath) {
    EnsureRealDllLoaded();
    if (g_pfnSHGetFolderPathA) {
        return g_pfnSHGetFolderPathA(hwnd, csidl, hToken, dwFlags, pszPath);
    }
    return E_FAIL;
}

extern "C" HRESULT WINAPI SHGetFolderPathW(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPWSTR pszPath) {
    EnsureRealDllLoaded();
    if (g_pfnSHGetFolderPathW) {
        return g_pfnSHGetFolderPathW(hwnd, csidl, hToken, dwFlags, pszPath);
    }
    return E_FAIL;
}

void LogMessage(const char* fmt, ...) {
    FILE* f = NULL;
    fopen_s(&f, "Blueprint\\blueprint.log", "a");
    if (!f) return;
    va_list args;
    va_start(args, fmt);
    vfprintf(f, fmt, args);
    va_end(args);
    fclose(f);
}

DWORD WINAPI BlueprintThread(LPVOID lpParam) {
    LogMessage("=============================================\n");
    LogMessage("[Blueprint] Mod loaded! PID: %lu\n", GetCurrentProcessId());

    // Wait until the game window appears
    HWND hGame = NULL;
    for (int i = 0; i < 60; i++) {
        hGame = FindWindowA("FFwinClass", NULL);
        if (hGame) break;
        Sleep(500);
    }

    if (!hGame) {
        LogMessage("[Blueprint] FFwinClass window not found after 30s. Exiting thread.\n");
        return 0;
    }

    LogMessage("[Blueprint] Found game window: 0x%p. Initializing Blueprint & Overlay...\n", hGame);

    BlueprintManager::Instance().Initialize();
    LogMessage("[Blueprint] BlueprintManager initialized with %d buildings.\n", 
               BlueprintManager::Instance().GetTotalCount());

    if (OverlayWindow::Instance().Initialize(g_hInstance)) {
        LogMessage("[Blueprint] OverlayWindow created successfully. Starting render loop.\n");
        OverlayWindow::Instance().RunLoop();
    } else {
        LogMessage("[Blueprint] Failed to create OverlayWindow!\n");
    }

    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        g_hInstance = hinstDLL;
        DisableThreadLibraryCalls(hinstDLL);
        CreateThread(NULL, 0, BlueprintThread, NULL, 0, NULL);
    } else if (fdwReason == DLL_PROCESS_DETACH) {
        OverlayWindow::Instance().Shutdown();
    }
    return TRUE;
}
