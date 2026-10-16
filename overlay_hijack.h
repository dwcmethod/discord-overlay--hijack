#pragma once
#include <windows.h>

// Discord's known overlay trap class name. If they rename it in a future
// build, update this — EnumWindows below will log candidates.
#define DISCORD_TRAP_CLASS L"DiscordDesktopOverlayInputTrap"

// Original WndProc storage
typedef LRESULT(WINAPI* WNDPROC_T)(HWND, UINT, WPARAM, LPARAM);
extern WNDPROC_T g_origWndProc;
extern HWND       g_hTrap;

LRESULT CALLBACK HijackWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL             InstallOverlayHijack();
void             RemoveOverlayHijack();