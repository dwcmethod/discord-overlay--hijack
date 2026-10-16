#include "overlay_hijack.h"
#include <cstdio>

WNDPROC_T g_origWndProc = nullptr;
HWND      g_hTrap        = nullptr;

// ---- your input handler ---------------------------------------------------
// This is where you do whatever you want with the raw input before Discord
// sees it. Return TRUE to swallow, FALSE to let it fall through to Discord.
static BOOL OnInput(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        // wParam = VK code, lParam = scan/repeat/flags
        // Example: swallow F8 so the game/Discord never see it.
        if (wParam == VK_F8) {
            // TODO: your action here
            return TRUE; // eaten
        }
        break;

    case WM_INPUT:
    {
        // Raw input from RIDEV_INPUTSINK. lParam = HRAWINPUT.
        UINT size = 0;
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));
        if (size && size <= 256)
        {
            BYTE buf[256];
            if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, buf, &size,
                                sizeof(RAWINPUTHEADER)) == size)
            {
                RAWINPUT* ri = (RAWINPUT*)buf;
                if (ri->header.dwType == RIM_TYPEKEYBOARD)
                {
                    USHORT vk = ri->data.keyboard.VKey;
                    // Raw input is where Discord actually reads from when the
                    // overlay is up. Hook here for keystrokes Discord will see.
                    (void)vk;
                }
                else if (ri->header.dwType == RIM_TYPEMOUSE)
                {
                    // ri->data.mouse.lLastX / lLastY / usButtonFlags
                }
            }
        }
        break;
    }

    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MOUSEWHEEL:
        // Cursor pos in lParam, buttons in wParam. Same deal.
        break;
    }
    return FALSE;
}
// ---------------------------------------------------------------------------

LRESULT CALLBACK HijackWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (OnInput(hWnd, msg, wParam, lParam))
        return 0; // swallowed

    // Chain to Discord's real handler so we don't break the overlay.
    if (g_origWndProc)
        return CallWindowProcW(g_origWndProc, hWnd, msg, wParam, lParam);

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// Find the trap window in our own process (we're injected into Discord.exe).
static BOOL CALLBACK EnumProc(HWND hWnd, LPARAM lParam)
{
    wchar_t cls[256] = {0};
    GetClassNameW(hWnd, cls, 255);

    if (wcscmp(cls, DISCORD_TRAP_CLASS) == 0)
    {
        *(HWND*)lParam = hWnd;
        return FALSE; // stop
    }
    return TRUE;
}

BOOL InstallOverlayHijack()
{
    if (g_hTrap) return TRUE;

    HWND found = nullptr;
    EnumWindows(EnumProc, (LPARAM)&found);
    if (!found) return FALSE;

    g_hTrap = found;

    // Subclass: swap WndProc, keep the original.
    SetLastError(0);
    LONG_PTR prev = SetWindowLongPtrW(
        g_hTrap,
        GWLP_WNDPROC,
        (LONG_PTR)HijackWndProc);

    if (!prev && GetLastError() != 0)
        return FALSE;

    g_origWndProc = (WNDPROC_T)prev;
    return TRUE;
}

void RemoveOverlayHijack()
{
    if (!g_hTrap || !g_origWndProc) return;
    SetWindowLongPtrW(g_hTrap, GWLP_WNDPROC, (LONG_PTR)g_origWndProc);
    g_origWndProc = nullptr;
    g_hTrap = nullptr;
}