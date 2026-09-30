#pragma once
#include <windows.h>

class OverlayWindow {
public:
    static OverlayWindow& Instance() {
        static OverlayWindow instance;
        return instance;
    }

    bool Initialize(HINSTANCE hInstance);
    void RunLoop();
    void Shutdown();

private:
    OverlayWindow();
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void Render();
    void HandleHotkeys();

    HWND m_hWnd;
    HWND m_hGameWnd;
    HINSTANCE m_hInstance;
    bool m_running;

    bool m_aimWithMouse; // true: Blueprint follows mouse cursor (F9), false: Screen center
    bool m_groundPinned; // true: Pinned to map ground, false: Free (Aiming)
    int m_pinnedWorldX;
    int m_pinnedWorldY;
    bool m_guidedMode; // true: Guided (Place Here), false: Classic (Detailed)
    bool m_guidedFullCastle; // true: Show entire castle in guided style, false: Step focus

    // Double buffering & fonts
    HDC m_hdcMem;
    HBITMAP m_hbmMem;
    HFONT m_hFont;
    HFONT m_hSmallFont;
    int m_width;
    int m_height;
};
