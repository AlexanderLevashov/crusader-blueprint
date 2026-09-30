#include "overlay.h"
#include "blueprint.h"
#include <stdio.h>

static bool GetGameCameraWorldPixel(int& camPixelX, int& camPixelY) {
    camPixelX = 0;
    camPixelY = 0;
    __try {
        // 1. Crusader HD camera world pixel position (from scrolling function at 0x4341b0)
        int px = *(int*)0x21aec50;
        int py = *(int*)0x21aec54;
        if (px != 0 || py != 0) {
            camPixelX = px;
            camPixelY = py;
            return true;
        }

        // 2. Crusader Extreme HD camera world pixel position (from scrolling function at 0x4343f0)
        px = *(int*)0x2c42150;
        py = *(int*)0x2c42154;
        if (px != 0 || py != 0) {
            camPixelX = px;
            camPixelY = py;
            return true;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        camPixelX = 0;
        camPixelY = 0;
    }
    return false;
}

OverlayWindow::OverlayWindow()
    : m_hWnd(NULL)
    , m_hGameWnd(NULL)
    , m_hInstance(NULL)
    , m_running(false)
    , m_aimWithMouse(false) // Default to Center Aim; F9 toggles Mouse Follow
    , m_groundPinned(false) // Default to Free Aiming mode!
    , m_pinnedWorldX(0)
    , m_pinnedWorldY(0)
    , m_guidedMode(true) // Default to Guided (Place Here)
    , m_guidedFullCastle(false) // Default to Step Focus
    , m_hdcMem(NULL)
    , m_hbmMem(NULL)
    , m_hFont(NULL)
    , m_hSmallFont(NULL)
    , m_width(0)
    , m_height(0)
{
}

LRESULT CALLBACK OverlayWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool OverlayWindow::Initialize(HINSTANCE hInstance) {
    m_hInstance = hInstance;

    WNDCLASSA wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "CrusaderBlueprintOverlayClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassA(&wc);

    m_hWnd = CreateWindowExA(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        wc.lpszClassName,
        "Crusader Blueprint Overlay",
        WS_POPUP,
        0, 0, 800, 600,
        NULL, NULL, hInstance, NULL
    );

    if (!m_hWnd) return false;

    // Crisp standard fonts for clean English UI
    m_hFont = CreateFontA(
        13, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Tahoma"
    );

    m_hSmallFont = CreateFontA(
        10, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Tahoma"
    );

    // Pure black (RGB 0,0,0) is 100% transparent and click-through.
    // LWA_ALPHA (200/255 = ~78% opacity) gives a sleek translucent holographic blueprint!
    SetLayeredWindowAttributes(m_hWnd, RGB(0, 0, 0), 200, LWA_COLORKEY | LWA_ALPHA);
    ShowWindow(m_hWnd, SW_SHOW);
    UpdateWindow(m_hWnd);

    m_running = true;
    return true;
}

void OverlayWindow::HandleHotkeys() {
    static DWORD lastPressToggle = 0;
    static DWORD lastPressLord   = 0;
    static DWORD lastPressVar    = 0;
    static DWORD lastPressPrev   = 0;
    static DWORD lastPressNext   = 0;
    static DWORD lastPressAll    = 0;
    static DWORD lastPressAim    = 0;
    static DWORD lastPressPin    = 0;
    static DWORD lastPressFocus  = 0;
    static DWORD lastPressStyle  = 0;

    DWORD now = GetTickCount();
    bool isShift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

    // [~] (Tilde): Toggle Blueprint ON / OFF
    if ((GetAsyncKeyState(VK_OEM_3) & 0x8000) && (now - lastPressToggle > 300)) {
        lastPressToggle = now;
        BlueprintManager::Instance().ToggleEnabled();
    }

    // [K] or [Home]: Cycle AI Lord (forward, or backward with Shift)
    if (((GetAsyncKeyState('K') & 0x8000) || (GetAsyncKeyState(VK_HOME) & 0x8000)) && (now - lastPressLord > 300)) {
        lastPressLord = now;
        if (isShift) {
            BlueprintManager::Instance().PrevCharacter();
        } else {
            BlueprintManager::Instance().NextCharacter();
        }
        LogMessage("[Blueprint] Lord switched to '%s'\n",
                   BlueprintManager::Instance().GetCurrentCharacter());
    }

    // [L] or [End]: Cycle Castle Variant 1..8 (forward, or backward with Shift)
    if (((GetAsyncKeyState('L') & 0x8000) || (GetAsyncKeyState(VK_END) & 0x8000)) && (now - lastPressVar > 300)) {
        lastPressVar = now;
        if (isShift) {
            BlueprintManager::Instance().PrevCastleVariant();
        } else {
            BlueprintManager::Instance().NextCastleVariant();
        }
        LogMessage("[Blueprint] Castle variant switched to #%d\n",
                   BlueprintManager::Instance().GetCurrentVariant());
    }

    // [ [ ] or [Page Up]: Previous Step
    if (((GetAsyncKeyState(VK_OEM_4) & 0x8000) || (GetAsyncKeyState(VK_PRIOR) & 0x8000)) && (now - lastPressPrev > 250)) {
        lastPressPrev = now;
        BlueprintManager::Instance().PrevStep();
    }

    // [ ] ] or [Page Down]: Next Step
    if (((GetAsyncKeyState(VK_OEM_6) & 0x8000) || (GetAsyncKeyState(VK_NEXT) & 0x8000)) && (now - lastPressNext > 250)) {
        lastPressNext = now;
        BlueprintManager::Instance().NextStep();
    }

    // [\] (Backslash): Toggle Show All Steps
    if ((GetAsyncKeyState(VK_OEM_5) & 0x8000) && (now - lastPressAll > 300)) {
        lastPressAll = now;
        BlueprintManager::Instance().ToggleShowAll();
    }

    // [T] or [Insert]: Toggle Aim mode (MOUSE FOLLOW / CENTER)
    if (((GetAsyncKeyState('T') & 0x8000) || (GetAsyncKeyState(VK_INSERT) & 0x8000)) && (now - lastPressAim > 300)) {
        lastPressAim = now;
        m_aimWithMouse = !m_aimWithMouse;
        if (m_groundPinned) {
            m_groundPinned = false; // Unpin so user can aim immediately
        }
        LogMessage("[Blueprint] Aim mode switched to %s\n",
                   m_aimWithMouse ? "MOUSE FOLLOW (Cursor)" : "CENTER (Screen Center)");
    }

    // [Y] or [Delete]: Toggle Ground Pinning (PINNED / FREE)
    if (((GetAsyncKeyState('Y') & 0x8000) || (GetAsyncKeyState(VK_DELETE) & 0x8000)) && (now - lastPressPin > 300)) {
        lastPressPin = now;
        m_groundPinned = !m_groundPinned;
        if (m_groundPinned) {
            int camPx = 0, camPy = 0;
            GetGameCameraWorldPixel(camPx, camPy);
            POINT ptCursor;
            GetCursorPos(&ptCursor);
            if (m_hGameWnd) ScreenToClient(m_hGameWnd, &ptCursor);
            int curX = m_aimWithMouse ? ptCursor.x : (m_width > 0 ? (m_width / 2) : 400);
            int curY = m_aimWithMouse ? ptCursor.y : (m_height > 0 ? (m_height / 2) : 300);
            m_pinnedWorldX = curX + camPx;
            m_pinnedWorldY = curY + camPy;
            LogMessage("[Blueprint] PINNED to terrain at world pixel (%d, %d)\n", m_pinnedWorldX, m_pinnedWorldY);
        } else {
            LogMessage("[Blueprint] UNPINNED, back to Free Aim\n");
        }
    }

    // [U]: Toggle Guided Full Castle / Step Focus
    if ((GetAsyncKeyState('U') & 0x8000) && (now - lastPressFocus > 300)) {
        lastPressFocus = now;
        m_guidedFullCastle = !m_guidedFullCastle;
        m_guidedMode = true; // Switch to Guided mode so user sees full castle immediately
        LogMessage("[Blueprint] Guided Full Castle: %s\n",
                   m_guidedFullCastle ? "ON (Entire Castle)" : "OFF (Step Focus)");
    }

    // [J]: Toggle Visual Style (Guided / Classic)
    if ((GetAsyncKeyState('J') & 0x8000) && (now - lastPressStyle > 300)) {
        lastPressStyle = now;
        m_guidedMode = !m_guidedMode;
        LogMessage("[Blueprint] Visual style: %s\n",
                   m_guidedMode ? "Guided (Place Here)" : "Classic (Detailed)");
    }
}


void OverlayWindow::Render() {
    if (!m_hGameWnd || !IsWindow(m_hGameWnd)) {
        m_hGameWnd = FindWindowA("FFwinClass", NULL);
        if (!m_hGameWnd) return;
    }

    // Keep overlay window positioned over game window
    RECT rcClient;
    GetClientRect(m_hGameWnd, &rcClient);
    POINT ptTopLeft = { rcClient.left, rcClient.top };
    ClientToScreen(m_hGameWnd, &ptTopLeft);

    int w = rcClient.right - rcClient.left;
    int h = rcClient.bottom - rcClient.top;

    if (w <= 0 || h <= 0) return;

    SetWindowPos(m_hWnd, HWND_TOPMOST, ptTopLeft.x, ptTopLeft.y, w, h, SWP_NOACTIVATE | SWP_SHOWWINDOW);

    // Double buffer allocation if resized
    if (!m_hdcMem || w != m_width || h != m_height) {
        if (m_hdcMem) DeleteDC(m_hdcMem);
        if (m_hbmMem) DeleteObject(m_hbmMem);

        HDC hdcScreen = GetDC(m_hWnd);
        m_hdcMem = CreateCompatibleDC(hdcScreen);
        m_hbmMem = CreateCompatibleBitmap(hdcScreen, w, h);
        SelectObject(m_hdcMem, m_hbmMem);
        ReleaseDC(m_hWnd, hdcScreen);

        m_width = w;
        m_height = h;
    }

    // 1. Clear to pure black (transparent color-key)
    RECT rcMem = { 0, 0, w, h };
    HBRUSH hBlackBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
    FillRect(m_hdcMem, &rcMem, hBlackBrush);

    // 2. Update Blueprint state
    BlueprintManager& bpm = BlueprintManager::Instance();
    bpm.Update();

    // Mouse cursor coordinates for hover inspection
    POINT ptMouse;
    GetCursorPos(&ptMouse);
    ScreenToClient(m_hGameWnd, &ptMouse);

    // Only draw if game window is in foreground
    HWND hForeground = GetForegroundWindow();
    bool isGameActive = (hForeground == m_hGameWnd || hForeground == m_hWnd);

    if (isGameActive && bpm.IsEnabled()) {
        int camPx = 0, camPy = 0;
        bool hasCam = GetGameCameraWorldPixel(camPx, camPy);

        int ksx = 0, ksy = 0;
        if (m_groundPinned && hasCam) {
            ksx = m_pinnedWorldX - camPx;
            ksy = m_pinnedWorldY - camPy;
        } else if (m_aimWithMouse) {
            ksx = ptMouse.x;
            ksy = ptMouse.y;
        } else {
            ksx = w / 2;
            ksy = h / 2;
        }

        int hoverBuildingId = -1;
        int hoverStep = -1;
        int hoverDistSq = 24 * 24;

        int currentStep = bpm.GetCurrentStep();
        bool hasKeepNow = bpm.HasKeep();

        if (m_guidedMode) {
            int activeMinSx = 999999, activeMaxSx = -999999;
            int activeMinSy = 999999, activeMaxSy = -999999;
            int activeBuildingId = -1;
            int activeTileCount = 0;

            DWORD tick = GetTickCount();
            int pulse = (int)(abs((long)((tick / 8) % 100) - 50)); // 0..50
            COLORREF activeColor = RGB(255, 205 + pulse, 30 + pulse * 2); // Vivid pulsating amber/gold

            // Pass 1: Future Steps
            if (m_guidedFullCastle) {
                // Full Castle Guided Mode: vibrant category colors, 100% hollow transparent interior!
                for (const auto& entry : bpm.GetEntries()) {
                    if (entry.step <= currentStep) continue;

                    if (hasKeepNow) {
                        int mapX = bpm.GetPlayerKeepX() + entry.dx;
                        int mapY = bpm.GetPlayerKeepY() + entry.dy;
                        if (bpm.IsTileBuilt(mapX, mapY, entry.buildingId)) continue;
                    }

                    int sx = ksx + (entry.dx - entry.dy) * 16;
                    int sy = ksy + (entry.dx + entry.dy) * 8;

                        // Hover check
                        int mdx = ptMouse.x - sx;
                        int mdy = ptMouse.y - sy;
                        int curDistSq = mdx * mdx + mdy * mdy;
                        if (curDistSq < hoverDistSq) {
                            hoverDistSq = curDistSq;
                            hoverBuildingId = entry.buildingId;
                            hoverStep = entry.step;
                        }

                        if (sx < -32 || sx > w + 32 || sy < -32 || sy > h + 32) continue;

                        COLORREF col = GetBuildingColor(entry.buildingId);
                        HPEN hColPen = CreatePen(PS_SOLID, 1, col);
                        HPEN hOldP = (HPEN)SelectObject(m_hdcMem, hColPen);
                        HBRUSH hOldB = (HBRUSH)SelectObject(m_hdcMem, GetStockObject(NULL_BRUSH));

                        POINT diamond[4] = {
                            { sx,      sy - 8 },
                            { sx + 16, sy     },
                            { sx,      sy + 8 },
                            { sx - 16, sy     }
                        };
                        Polygon(m_hdcMem, diamond, 4);

                        SelectObject(m_hdcMem, hOldB);
                        SelectObject(m_hdcMem, hOldP);
                        DeleteObject(hColPen);

                        // Clean tag for special buildings (not walls)
                        const char* shortLbl = GetBuildingShortLabel(entry.buildingId);
                        if (shortLbl) {
                            SelectObject(m_hdcMem, m_hSmallFont);
                            SIZE sz;
                            GetTextExtentPoint32A(m_hdcMem, shortLbl, (int)strlen(shortLbl), &sz);

                            RECT rcTag = { sx - sz.cx / 2 - 3, sy - sz.cy / 2 - 1, 
                                           sx + sz.cx / 2 + 3, sy + sz.cy / 2 + 1 };

                            HBRUSH hTagBg = CreateSolidBrush(RGB(15, 18, 24));
                            HPEN hTagBorder = CreatePen(PS_SOLID, 1, col);
                            HBRUSH hOldTB = (HBRUSH)SelectObject(m_hdcMem, hTagBg);
                            HPEN hOldTP = (HPEN)SelectObject(m_hdcMem, hTagBorder);

                            RoundRect(m_hdcMem, rcTag.left, rcTag.top, rcTag.right, rcTag.bottom, 4, 4);

                            SelectObject(m_hdcMem, hOldTB); DeleteObject(hTagBg);
                            SelectObject(m_hdcMem, hOldTP); DeleteObject(hTagBorder);

                            SetBkMode(m_hdcMem, TRANSPARENT);
                            SetTextColor(m_hdcMem, RGB(220, 220, 220));
                            TextOutA(m_hdcMem, sx - sz.cx / 2, sy - sz.cy / 2, shortLbl, (int)strlen(shortLbl));
                        }
                    }
                } else if (bpm.IsShowAll()) {
                    // Step Focus: Future Steps are dim ghost wireframe, NO text clutter, hollow transparent fill
                    HPEN hGhostPen = CreatePen(PS_SOLID, 1, RGB(45, 68, 88));
                    HPEN hOldPen = (HPEN)SelectObject(m_hdcMem, hGhostPen);
                    HBRUSH hOldBrush = (HBRUSH)SelectObject(m_hdcMem, GetStockObject(NULL_BRUSH));

                    for (const auto& entry : bpm.GetEntries()) {
                        if (entry.step <= currentStep) continue;

                        if (hasKeepNow) {
                            int mapX = bpm.GetPlayerKeepX() + entry.dx;
                            int mapY = bpm.GetPlayerKeepY() + entry.dy;
                            if (bpm.IsTileBuilt(mapX, mapY, entry.buildingId)) continue;
                        }

                        int sx = ksx + (entry.dx - entry.dy) * 16;
                        int sy = ksy + (entry.dx + entry.dy) * 8;

                        // Hover check
                        int mdx = ptMouse.x - sx;
                        int mdy = ptMouse.y - sy;
                        int curDistSq = mdx * mdx + mdy * mdy;
                        if (curDistSq < hoverDistSq) {
                            hoverDistSq = curDistSq;
                            hoverBuildingId = entry.buildingId;
                            hoverStep = entry.step;
                        }

                        if (sx < -32 || sx > w + 32 || sy < -32 || sy > h + 32) continue;

                        POINT diamond[4] = {
                            { sx,      sy - 8 },
                            { sx + 16, sy     },
                            { sx,      sy + 8 },
                            { sx - 16, sy     }
                        };
                        Polygon(m_hdcMem, diamond, 4);
                    }

                    SelectObject(m_hdcMem, hOldBrush);
                    SelectObject(m_hdcMem, hOldPen);
                    DeleteObject(hGhostPen);
                }

                // Pass 2: Active Step (Bright pulsing gold outline + clean tile labels)
                {
                    HPEN hActivePen = CreatePen(PS_SOLID, 2, activeColor);
                    HPEN hOldPen = (HPEN)SelectObject(m_hdcMem, hActivePen);
                    HBRUSH hOldBrush = (HBRUSH)SelectObject(m_hdcMem, GetStockObject(NULL_BRUSH));

                    for (const auto& entry : bpm.GetEntries()) {
                        if (entry.step != currentStep) continue;

                        if (hasKeepNow) {
                            int mapX = bpm.GetPlayerKeepX() + entry.dx;
                            int mapY = bpm.GetPlayerKeepY() + entry.dy;
                            if (bpm.IsTileBuilt(mapX, mapY, entry.buildingId)) continue;
                        }

                        int sx = ksx + (entry.dx - entry.dy) * 16;
                        int sy = ksy + (entry.dx + entry.dy) * 8;

                        // Hover check with priority for active step
                        int mdx = ptMouse.x - sx;
                        int mdy = ptMouse.y - sy;
                        int curDistSq = mdx * mdx + mdy * mdy;
                        if (curDistSq < hoverDistSq * 3) {
                            hoverDistSq = curDistSq / 3;
                            hoverBuildingId = entry.buildingId;
                            hoverStep = entry.step;
                        }

                        if (sx < -32 || sx > w + 32 || sy < -32 || sy > h + 32) continue;

                        if (sx < activeMinSx) activeMinSx = sx;
                        if (sx > activeMaxSx) activeMaxSx = sx;
                        if (sy < activeMinSy) activeMinSy = sy;
                        if (sy > activeMaxSy) activeMaxSy = sy;
                        if (activeBuildingId == 0 || entry.buildingId != BUILDING_TRAINING_GROUND) {
                            activeBuildingId = entry.buildingId;
                        }
                        activeTileCount++;

                        POINT diamond[4] = {
                            { sx,      sy - 8 },
                            { sx + 16, sy     },
                            { sx,      sy + 8 },
                            { sx - 16, sy     }
                        };
                        Polygon(m_hdcMem, diamond, 4);

                        // Short label on active step tiles
                        const char* shortLbl = GetBuildingShortLabel(entry.buildingId);
                        if (shortLbl) {
                            SelectObject(m_hdcMem, m_hSmallFont);
                            SIZE sz;
                            GetTextExtentPoint32A(m_hdcMem, shortLbl, (int)strlen(shortLbl), &sz);

                            RECT rcTag = { sx - sz.cx / 2 - 3, sy - sz.cy / 2 - 1, 
                                           sx + sz.cx / 2 + 3, sy + sz.cy / 2 + 1 };

                            HBRUSH hTagBg = CreateSolidBrush(RGB(18, 22, 28));
                            HPEN hTagBorder = CreatePen(PS_SOLID, 1, activeColor);
                            HBRUSH hOldTB = (HBRUSH)SelectObject(m_hdcMem, hTagBg);
                            HPEN hOldTP = (HPEN)SelectObject(m_hdcMem, hTagBorder);

                            RoundRect(m_hdcMem, rcTag.left, rcTag.top, rcTag.right, rcTag.bottom, 4, 4);

                            SelectObject(m_hdcMem, hOldTB); DeleteObject(hTagBg);
                            SelectObject(m_hdcMem, hOldTP); DeleteObject(hTagBorder);

                            SetBkMode(m_hdcMem, TRANSPARENT);
                            SetTextColor(m_hdcMem, RGB(255, 235, 60));
                            TextOutA(m_hdcMem, sx - sz.cx / 2, sy - sz.cy / 2, shortLbl, (int)strlen(shortLbl));
                        }
                    }

                    SelectObject(m_hdcMem, hOldBrush);
                    SelectObject(m_hdcMem, hOldPen);
                    DeleteObject(hActivePen);
                }

                // Pass 3: Draw "★ PLACE HERE: <BuildingName> ★" Floating Badge & Target Brackets
                if (activeTileCount > 0 && activeBuildingId > 0) {
                    int centerX = (activeMinSx + activeMaxSx) / 2;
                    int centerY = (activeMinSy + activeMaxSy) / 2;
                    int boxLeft = activeMinSx - 18;
                    int boxTop = activeMinSy - 10;
                    int boxRight = activeMaxSx + 18;
                    int boxBottom = activeMaxSy + 10;

                    // 1. High-tech Corner Brackets ┌ ┐ └ ┘
                    int arm = 12;
                    HPEN hBracketPen = CreatePen(PS_SOLID, 2, activeColor);
                    HPEN hOldBP = (HPEN)SelectObject(m_hdcMem, hBracketPen);

                    // ┌ Top-left
                    MoveToEx(m_hdcMem, boxLeft, boxTop + arm, NULL);
                    LineTo(m_hdcMem, boxLeft, boxTop);
                    LineTo(m_hdcMem, boxLeft + arm, boxTop);

                    // ┐ Top-right
                    MoveToEx(m_hdcMem, boxRight - arm, boxTop, NULL);
                    LineTo(m_hdcMem, boxRight, boxTop);
                    LineTo(m_hdcMem, boxRight, boxTop + arm);

                    // └ Bottom-left
                    MoveToEx(m_hdcMem, boxLeft, boxBottom - arm, NULL);
                    LineTo(m_hdcMem, boxLeft, boxBottom);
                    LineTo(m_hdcMem, boxLeft + arm, boxBottom);

                    // ┘ Bottom-right
                    MoveToEx(m_hdcMem, boxRight - arm, boxBottom, NULL);
                    LineTo(m_hdcMem, boxRight, boxBottom);
                    LineTo(m_hdcMem, boxRight, boxBottom - arm);

                    SelectObject(m_hdcMem, hOldBP);
                    DeleteObject(hBracketPen);

                    // 2. Floating callout badge: "★ PLACE HERE: <BuildingName> ★"
                    const char* bldName = GetBuildingName(activeBuildingId);
                    char badgeText[128];
                    sprintf_s(badgeText, "★ PLACE HERE: %s (Step %d) ★", bldName, currentStep);

                    SelectObject(m_hdcMem, m_hFont);
                    SIZE bsz;
                    GetTextExtentPoint32A(m_hdcMem, badgeText, (int)strlen(badgeText), &bsz);

                    int badgeW = bsz.cx + 20;
                    int badgeH = bsz.cy + 10;
                    int badgeX = centerX - badgeW / 2;
                    int badgeY = boxTop - badgeH - 12;

                    bool arrowPointsDown = true;
                    // Avoid top HUD (x: 15..530, y: 15..145)
                    if (badgeY < 155 && badgeX < 540) {
                        badgeY = boxBottom + 14;
                        arrowPointsDown = false;
                    }
                    if (badgeY < 15) {
                        badgeY = boxBottom + 14;
                        arrowPointsDown = false;
                    }
                    if (badgeX < 15) badgeX = 15;
                    if (badgeX + badgeW > w - 15) badgeX = w - badgeW - 15;

                    RECT rcBadge = { badgeX, badgeY, badgeX + badgeW, badgeY + badgeH };
                    HBRUSH hBadgeBg = CreateSolidBrush(RGB(18, 22, 28));
                    HPEN hBadgeBorder = CreatePen(PS_SOLID, 2, activeColor);
                    HBRUSH hOldBB = (HBRUSH)SelectObject(m_hdcMem, hBadgeBg);
                    HPEN hOldBBP = (HPEN)SelectObject(m_hdcMem, hBadgeBorder);

                    RoundRect(m_hdcMem, rcBadge.left, rcBadge.top, rcBadge.right, rcBadge.bottom, 6, 6);

                    SelectObject(m_hdcMem, hOldBB); DeleteObject(hBadgeBg);
                    SelectObject(m_hdcMem, hOldBBP); DeleteObject(hBadgeBorder);

                    // Pointer arrow
                    POINT arrow[3];
                    if (arrowPointsDown) {
                        arrow[0] = { centerX - 8, badgeY + badgeH };
                        arrow[1] = { centerX + 8, badgeY + badgeH };
                        arrow[2] = { centerX,     boxTop - 2 };
                    } else {
                        arrow[0] = { centerX - 8, badgeY };
                        arrow[1] = { centerX + 8, badgeY };
                        arrow[2] = { centerX,     boxBottom + 2 };
                    }

                    HPEN hArrowPen = CreatePen(PS_SOLID, 1, activeColor);
                    HBRUSH hArrowBrush = CreateSolidBrush(activeColor);
                    HPEN hOldAP = (HPEN)SelectObject(m_hdcMem, hArrowPen);
                    HBRUSH hOldAB = (HBRUSH)SelectObject(m_hdcMem, hArrowBrush);
                    Polygon(m_hdcMem, arrow, 3);
                    SelectObject(m_hdcMem, hOldAB); DeleteObject(hArrowBrush);
                    SelectObject(m_hdcMem, hOldAP); DeleteObject(hArrowPen);

                    SetBkMode(m_hdcMem, TRANSPARENT);
                    SetTextColor(m_hdcMem, RGB(255, 235, 60));
                    TextOutA(m_hdcMem, badgeX + 10, badgeY + 5, badgeText, (int)strlen(badgeText));
                }
            } else {
                // Classic Mode: all steps (or up to current step if !m_showAll)
                for (const auto& entry : bpm.GetEntries()) {
                    if (!bpm.IsShowAll() && entry.step > currentStep) continue;

                    if (hasKeepNow) {
                        int mapX = bpm.GetPlayerKeepX() + entry.dx;
                        int mapY = bpm.GetPlayerKeepY() + entry.dy;
                        if (bpm.IsTileBuilt(mapX, mapY, entry.buildingId)) continue;
                    }

                    int sx = ksx + (entry.dx - entry.dy) * 16;
                    int sy = ksy + (entry.dx + entry.dy) * 8;

                    // Hover check
                    int mdx = ptMouse.x - sx;
                    int mdy = ptMouse.y - sy;
                    int curDistSq = mdx * mdx + mdy * mdy;
                    if (curDistSq < hoverDistSq) {
                        hoverDistSq = curDistSq;
                        hoverBuildingId = entry.buildingId;
                        hoverStep = entry.step;
                    }

                    if (sx < -32 || sx > w + 32 || sy < -32 || sy > h + 32) continue;

                    COLORREF col = GetBuildingColor(entry.buildingId);
                    HPEN hPen = CreatePen(PS_SOLID, 2, col);
                    HPEN hOldPen = (HPEN)SelectObject(m_hdcMem, hPen);

                    HBRUSH hBrush = CreateSolidBrush(RGB(GetRValue(col) / 5, GetGValue(col) / 5, GetBValue(col) / 5));
                    HBRUSH hOldBrush = (HBRUSH)SelectObject(m_hdcMem, hBrush);

                    POINT diamond[4] = {
                        { sx,      sy - 8 },
                        { sx + 16, sy     },
                        { sx,      sy + 8 },
                        { sx - 16, sy     }
                    };

                    Polygon(m_hdcMem, diamond, 4);

                    SelectObject(m_hdcMem, hOldBrush);
                    DeleteObject(hBrush);
                    SelectObject(m_hdcMem, hOldPen);
                    DeleteObject(hPen);

                    // Draw Building Label (Clean English abbreviation on tile)
                    const char* shortLbl = GetBuildingShortLabel(entry.buildingId);
                    if (shortLbl) {
                        SelectObject(m_hdcMem, m_hSmallFont);
                        SIZE sz;
                        GetTextExtentPoint32A(m_hdcMem, shortLbl, (int)strlen(shortLbl), &sz);

                        RECT rcTag = { sx - sz.cx / 2 - 3, sy - sz.cy / 2 - 1, 
                                       sx + sz.cx / 2 + 3, sy + sz.cy / 2 + 1 };

                        HBRUSH hTagBg = CreateSolidBrush(RGB(15, 18, 24));
                        HPEN hTagBorder = CreatePen(PS_SOLID, 1, col);
                        HBRUSH hOldTB = (HBRUSH)SelectObject(m_hdcMem, hTagBg);
                        HPEN hOldTP = (HPEN)SelectObject(m_hdcMem, hTagBorder);

                        RoundRect(m_hdcMem, rcTag.left, rcTag.top, rcTag.right, rcTag.bottom, 4, 4);

                        SelectObject(m_hdcMem, hOldTB); DeleteObject(hTagBg);
                        SelectObject(m_hdcMem, hOldTP); DeleteObject(hTagBorder);

                        SetBkMode(m_hdcMem, TRANSPARENT);
                        SetTextColor(m_hdcMem, RGB(255, 255, 255));
                        TextOutA(m_hdcMem, sx - sz.cx / 2, sy - sz.cy / 2, shortLbl, (int)strlen(shortLbl));
                    }
                }
            }

            // 2c. Draw Keep Anchor Marker (Gold indicator + text)
            if (ksx >= -80 && ksx <= w + 80 && ksy >= -80 && ksy <= h + 80) {
                COLORREF keepCol = RGB(255, 215, 0);

                HPEN hKeepPen = CreatePen(PS_SOLID, 3, keepCol);
                HPEN hOldPen = (HPEN)SelectObject(m_hdcMem, hKeepPen);
                HBRUSH hOldBrush = (HBRUSH)SelectObject(m_hdcMem, GetStockObject(NULL_BRUSH));

                POINT keepDiamond[4] = {
                    { ksx,      ksy - 14 },
                    { ksx + 28, ksy      },
                    { ksx,      ksy + 14 },
                    { ksx - 28, ksy      }
                };
                Polygon(m_hdcMem, keepDiamond, 4);

                SelectObject(m_hdcMem, hOldBrush);
                SelectObject(m_hdcMem, hOldPen);
                DeleteObject(hKeepPen);

                char keepMsg[64];
                if (hasKeepNow) {
                    strcpy_s(keepMsg, "KEEP");
                } else if (m_groundPinned) {
                    strcpy_s(keepMsg, "★ PLACE KEEP HERE ★");
                } else if (m_aimWithMouse) {
                    strcpy_s(keepMsg, "★ MOUSE AIM [Y: PIN] ★");
                } else {
                    strcpy_s(keepMsg, "★ CENTER AIM [Y: PIN] ★");
                }

                SelectObject(m_hdcMem, m_hFont);
                SIZE ksz;
                GetTextExtentPoint32A(m_hdcMem, keepMsg, (int)strlen(keepMsg), &ksz);

                int kbW = ksz.cx + 16;
                int kbH = ksz.cy + 8;
                int kbX = ksx - kbW / 2;
                int kbY = ksy - 30;

                RECT rcKB = { kbX, kbY, kbX + kbW, kbY + kbH };
                HBRUSH hKBBg = CreateSolidBrush(RGB(18, 22, 28));
                HPEN hKBBorder = CreatePen(PS_SOLID, 1, keepCol);
                HBRUSH hOldKB = (HBRUSH)SelectObject(m_hdcMem, hKBBg);
                HPEN hOldKBP = (HPEN)SelectObject(m_hdcMem, hKBBorder);

                RoundRect(m_hdcMem, rcKB.left, rcKB.top, rcKB.right, rcKB.bottom, 6, 6);

                SelectObject(m_hdcMem, hOldKB); DeleteObject(hKBBg);
                SelectObject(m_hdcMem, hOldKBP); DeleteObject(hKBBorder);

                SetBkMode(m_hdcMem, TRANSPARENT);
                SetTextColor(m_hdcMem, keepCol);
                TextOutA(m_hdcMem, kbX + 8, kbY + 4, keepMsg, (int)strlen(keepMsg));
            }

        // 3. Hover Tooltip under mouse
        if (hoverBuildingId > 0 && ptMouse.x >= 0 && ptMouse.x <= w && ptMouse.y >= 0 && ptMouse.y <= h) {
            const char* fullName = GetBuildingName(hoverBuildingId);
            char tipBuf[128];
            if (m_guidedMode) {
                if (hoverStep == currentStep) {
                    sprintf_s(tipBuf, " ★ PLACE HERE: %s (Step %d) ★ ", fullName, hoverStep);
                } else {
                    sprintf_s(tipBuf, " %s (Step %d - Future) ", fullName, hoverStep);
                }
            } else {
                sprintf_s(tipBuf, " %s (Step %d) ", fullName, hoverStep);
            }

            SelectObject(m_hdcMem, m_hFont);
            SIZE sz;
            GetTextExtentPoint32A(m_hdcMem, tipBuf, (int)strlen(tipBuf), &sz);

            int tx = ptMouse.x + 18;
            int ty = ptMouse.y + 18;
            if (tx + sz.cx + 10 > w) tx = ptMouse.x - sz.cx - 15;
            if (ty + sz.cy + 10 > h) ty = ptMouse.y - sz.cy - 15;

            RECT rcTip = { tx, ty, tx + sz.cx + 8, ty + sz.cy + 6 };
            HBRUSH hTipBg = CreateSolidBrush(RGB(20, 24, 32));
            HPEN hTipPen = CreatePen(PS_SOLID, 1, (m_guidedMode && hoverStep == currentStep) ? RGB(255, 215, 0) : RGB(0, 220, 255));
            HBRUSH hOldTB = (HBRUSH)SelectObject(m_hdcMem, hTipBg);
            HPEN hOldTP = (HPEN)SelectObject(m_hdcMem, hTipPen);

            RoundRect(m_hdcMem, rcTip.left, rcTip.top, rcTip.right, rcTip.bottom, 6, 6);

            SelectObject(m_hdcMem, hOldTB); DeleteObject(hTipBg);
            SelectObject(m_hdcMem, hOldTP); DeleteObject(hTipPen);

            SetBkMode(m_hdcMem, TRANSPARENT);
            SetTextColor(m_hdcMem, (m_guidedMode && hoverStep == currentStep) ? RGB(255, 230, 80) : RGB(0, 240, 255));
            TextOutA(m_hdcMem, rcTip.left + 4, rcTip.top + 3, tipBuf, (int)strlen(tipBuf));
        }

        // 4. Draw HUD Banner in top-left
        RECT hudRect = { 15, 15, 540, 138 };
        HBRUSH hHudBg = CreateSolidBrush(RGB(15, 18, 24));
        HPEN hHudBorder = CreatePen(PS_SOLID, 1, RGB(0, 180, 255));
        HBRUSH hOldB = (HBRUSH)SelectObject(m_hdcMem, hHudBg);
        HPEN hOldP = (HPEN)SelectObject(m_hdcMem, hHudBorder);

        RoundRect(m_hdcMem, hudRect.left, hudRect.top, hudRect.right, hudRect.bottom, 8, 8);

        SelectObject(m_hdcMem, hOldB); DeleteObject(hHudBg);
        SelectObject(m_hdcMem, hOldP); DeleteObject(hHudBorder);

        SelectObject(m_hdcMem, m_hFont);
        SetBkMode(m_hdcMem, TRANSPARENT);

        char line1[128], line2[128], line3[128], line4[128], line5[128];
        sprintf_s(line1, "[~] Blueprint: ON | Castle: %s", bpm.GetCastleName().c_str());
        sprintf_s(line2, "[K] Lord: %s | [L] Variant: #%d of 8", 
                  bpm.GetCurrentCharacter(), bpm.GetCurrentVariant());

        if (hasKeepNow) {
            const char* targetName = bpm.GetCurrentStepBuildingName();
            sprintf_s(line3, "[ [ / ] ] Step %d/%d: %s | Built: %d/%d (%d%%)", 
                      bpm.GetCurrentStep(), bpm.GetMaxStep(), 
                      targetName,
                      bpm.GetBuiltCount(), bpm.GetTotalCount(),
                      bpm.GetTotalCount() > 0 ? (bpm.GetBuiltCount() * 100 / bpm.GetTotalCount()) : 0);
        } else {
            const char* targetName = bpm.GetCurrentStepBuildingName();
            sprintf_s(line3, "[ [ / ] ] Step %d/%d: %s | Keep not placed yet", 
                      bpm.GetCurrentStep(), bpm.GetMaxStep(), targetName);
        }

        sprintf_s(line4, "[T] Aim: %s | [Y] Ground: %s | [U] %s", 
                  m_aimWithMouse ? "MOUSE" : "CENTER",
                  m_groundPinned ? "PINNED" : "FREE",
                  m_guidedFullCastle ? "Full Castle" : "Step Focus");

        if (m_groundPinned) {
            sprintf_s(line5, "[J] %s | PINNED at Ground (%d,%d)", 
                      m_guidedMode ? "Guided" : "Classic",
                      m_pinnedWorldX, m_pinnedWorldY);
        } else if (m_aimWithMouse) {
            sprintf_s(line5, "[J] %s | Following Mouse -> Move to Keep/Corner & Press [Y]", 
                      m_guidedMode ? "Guided" : "Classic");
        } else {
            sprintf_s(line5, "[J] %s | Aim with Camera -> Press [T] for Mouse or [Y] to Pin", 
                      m_guidedMode ? "Guided" : "Classic");
        }

        SetTextColor(m_hdcMem, RGB(0, 220, 255));
        TextOutA(m_hdcMem, 25, 20, line1, (int)strlen(line1));

        SetTextColor(m_hdcMem, RGB(255, 215, 0));
        TextOutA(m_hdcMem, 25, 42, line2, (int)strlen(line2));

        SetTextColor(m_hdcMem, bpm.HasKeep() ? RGB(100, 255, 100) : RGB(255, 100, 100));
        TextOutA(m_hdcMem, 25, 64, line3, (int)strlen(line3));

        SetTextColor(m_hdcMem, RGB(180, 180, 180));
        TextOutA(m_hdcMem, 25, 86, line4, (int)strlen(line4));

        SetTextColor(m_hdcMem, RGB(120, 255, 120));
        TextOutA(m_hdcMem, 25, 108, line5, (int)strlen(line5));
    }

    // 5. Blit memory DC to overlay window
    HDC hdcWin = GetDC(m_hWnd);
    BitBlt(hdcWin, 0, 0, w, h, m_hdcMem, 0, 0, SRCCOPY);
    ReleaseDC(m_hWnd, hdcWin);
}

void OverlayWindow::RunLoop() {
    MSG msg;
    while (m_running) {
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                m_running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        HandleHotkeys();
        Render();
        Sleep(16); // ~60 FPS
    }
}

void OverlayWindow::Shutdown() {
    m_running = false;
    if (m_hFont) { DeleteObject(m_hFont); m_hFont = NULL; }
    if (m_hSmallFont) { DeleteObject(m_hSmallFont); m_hSmallFont = NULL; }
    if (m_hdcMem) { DeleteDC(m_hdcMem); m_hdcMem = NULL; }
    if (m_hbmMem) { DeleteObject(m_hbmMem); m_hbmMem = NULL; }
    if (m_hWnd) { DestroyWindow(m_hWnd); m_hWnd = NULL; }
}
