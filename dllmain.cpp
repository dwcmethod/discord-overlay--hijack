#include <windows.h>
#include "overlay_hijack.h"

static DWORD WINAPI InitThread(LPVOID)
{
    // Discord creates the trap lazily — retry until it shows up.
    for (int i = 0; i < 120; ++i) // ~60s
    {
        if (InstallOverlayHijack())
            break;
        Sleep(500);
    }
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hInst);
        HANDLE h = CreateThread(nullptr, 0, InitThread, nullptr, 0, nullptr);
        if (h) CloseHandle(h);
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        RemoveOverlayHijack();
    }
    return TRUE;
}