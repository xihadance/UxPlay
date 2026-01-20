/**
 * UxPlay - An open-source AirPlay mirroring server
 * Copyright (C) 2021-2024 F. Duncanh
 *
 * Windows window management for exclusive fullscreen mode
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */

#ifdef _WIN32

#include "windows_window.h"
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Static variables for window state */
static HWND g_hwnd = NULL;
static bool g_fullscreen = false;
static bool g_original_fullscreen = false;
static RECT g_original_window_rect;
static DWORD g_original_window_style;
static int g_fullscreen_toggle_mode = 1; /* 0 = off, 1 = Alt+Enter, 2 = F11 */
static HMONITOR g_current_monitor = NULL;
static DEVMODE g_original_display_mode;

/* Window class name */
static const char* WINDOW_CLASS_NAME = "UxPlayWindowClass";

/* Forward declarations */
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static bool change_display_settings_exclusive(int width, int height, int refresh_rate);
static HMONITOR get_monitor_for_window(HWND hwnd);
static bool is_primary_monitor(HMONITOR monitor);
static bool save_original_display_settings();
static bool restore_display_settings_internal();

/**
 * Create an exclusive fullscreen window
 */
HWND windows_create_fullscreen_window(int width, int height, const char *title) {
    HINSTANCE hInstance = GetModuleHandle(NULL);

    /* Register window class */
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = WINDOW_CLASS_NAME;

    if (!RegisterClassEx(&wc)) {
        DWORD error = GetLastError();
        if (error != ERROR_CLASS_ALREADY_EXISTS) {
            fprintf(stderr, "Failed to register window class: %lu\n", error);
            return NULL;
        }
    }

    /* Calculate window position for primary monitor */
    int screen_width = GetSystemMetrics(SM_CXSCREEN);
    int screen_height = GetSystemMetrics(SM_CYSCREEN);

    /* Save original display settings before changing them */
    save_original_display_settings();

    /* Create window with popup style for exclusive fullscreen */
    g_hwnd = CreateWindowEx(
        WS_EX_TOPMOST,  /* Always on top for exclusive fullscreen */
        WINDOW_CLASS_NAME,
        title ? title : "UxPlay",
        WS_POPUP,       /* No borders, no title bar */
        0,              /* x position */
        0,              /* y position */
        screen_width,   /* width (will be updated to match screen) */
        screen_height,  /* height (will be updated to match screen) */
        NULL,           /* parent window */
        NULL,           /* menu */
        hInstance,
        NULL
    );

    if (!g_hwnd) {
        fprintf(stderr, "Failed to create fullscreen window: %lu\n", GetLastError());
        return NULL;
    }

    /* Show the window */
    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);

    /* Set fullscreen state */
    g_fullscreen = true;
    g_original_fullscreen = false;

    /* Set focus to the window */
    SetFocus(g_hwnd);

    return g_hwnd;
}

/**
 * Get the current fullscreen window handle
 */
HWND windows_get_window_handle() {
    return g_hwnd;
}

/**
 * Check if we're in exclusive fullscreen mode
 */
bool windows_is_fullscreen_mode() {
    return g_fullscreen;
}

/**
 * Toggle between fullscreen and windowed mode
 */
bool windows_toggle_fullscreen() {
    if (!g_hwnd) {
        return false;
    }

    if (g_fullscreen) {
        /* Switch to windowed mode */
        if (g_original_window_rect.right > 0 && g_original_window_rect.bottom > 0) {
            /* Restore original window size and position */
            SetWindowLongPtr(g_hwnd, GWL_STYLE, g_original_window_style);
            SetWindowPos(g_hwnd, NULL,
                         g_original_window_rect.left,
                         g_original_window_rect.top,
                         g_original_window_rect.right - g_original_window_rect.left,
                         g_original_window_rect.bottom - g_original_window_rect.top,
                         SWP_FRAMECHANGED | SWP_NOZORDER | SWP_SHOWWINDOW);

            /* Restore display settings if we changed them */
            restore_display_settings_internal();

            g_fullscreen = false;

            /* Notify GStreamer to update its state */
            return true;
        }
    } else {
        /* Switch to fullscreen mode */
        /* Save current window state */
        GetWindowRect(g_hwnd, &g_original_window_rect);
        g_original_window_style = GetWindowLongPtr(g_hwnd, GWL_STYLE);
        g_original_fullscreen = true;

        /* Get screen dimensions */
        int screen_width = GetSystemMetrics(SM_CXSCREEN);
        int screen_height = GetSystemMetrics(SM_CYSCREEN);

        /* Switch to exclusive fullscreen */
        SetWindowLongPtr(g_hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowPos(g_hwnd, HWND_TOPMOST, 0, 0, screen_width, screen_height,
                     SWP_FRAMECHANGED | SWP_SHOWWINDOW);

        g_fullscreen = true;

        /* Notify GStreamer to update its state */
        return true;
    }

    return false;
}

/**
 * Restore display settings on exit
 */
void windows_restore_display_settings() {
    restore_display_settings_internal();
}

/**
 * Handle window messages
 */
LRESULT windows_handle_message(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    return WndProc(hwnd, msg, wParam, lParam);
}

/**
 * Cleanup Windows window resources
 */
void windows_cleanup() {
    if (g_hwnd) {
        /* Restore display settings before destroying window */
        restore_display_settings_internal();

        DestroyWindow(g_hwnd);
        g_hwnd = NULL;
    }

    /* Unregister window class */
    HINSTANCE hInstance = GetModuleHandle(NULL);
    UnregisterClass(WINDOW_CLASS_NAME, hInstance);
}

/**
 * Set the fullscreen toggle mode
 */
void windows_set_toggle_mode(int mode) {
    g_fullscreen_toggle_mode = mode;
}

/**
 * Window procedure
 */
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_KEYDOWN:
            /* Handle fullscreen toggle keys */
            if (g_fullscreen_toggle_mode == 1 && wParam == VK_RETURN && GetAsyncKeyState(VK_MENU) < 0) {
                /* Alt+Enter toggle */
                windows_toggle_fullscreen();
                return 0;
            } else if (g_fullscreen_toggle_mode == 2 && wParam == VK_F11) {
                /* F11 toggle */
                windows_toggle_fullscreen();
                return 0;
            }
            break;

        case WM_KEYUP:
            /* Clear Alt key state */
            if (wParam == VK_MENU) {
                /* Alt key released */
            }
            break;

        case WM_DESTROY:
            /* Handle window destruction */
            if (g_hwnd == hwnd) {
                restore_display_settings_internal();
                g_hwnd = NULL;
            }
            PostQuitMessage(0);
            return 0;

        case WM_CLOSE:
            /* Handle window close */
            if (g_fullscreen) {
                /* Restore display settings before closing */
                restore_display_settings_internal();
            }
            DestroyWindow(hwnd);
            return 0;

        case WM_ACTIVATE:
            /* Handle activation */
            if (g_fullscreen && wParam != WA_INACTIVE) {
                /* Ensure fullscreen mode is maintained when activated */
                SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                             SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            }
            break;

        case WM_SYSKEYDOWN:
            /* Handle system keys for fullscreen toggle */
            if (g_fullscreen_toggle_mode == 1 && wParam == VK_RETURN) {
                /* Alt+Enter toggle (system key) */
                windows_toggle_fullscreen();
                return 0;
            }
            break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/**
 * Change display settings for exclusive fullscreen mode
 */
static bool change_display_settings_exclusive(int width, int height, int refresh_rate) {
    if (!g_hwnd) {
        return false;
    }

    /* Get current display settings */
    DEVMODE dm;
    ZeroMemory(&dm, sizeof(dm));
    dm.dmSize = sizeof(dm);

    if (!EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm)) {
        fprintf(stderr, "Failed to enumerate display settings\n");
        return false;
    }

    /* Store original settings if not already stored */
    if (g_original_display_mode.dmSize == 0) {
        memcpy(&g_original_display_mode, &dm, sizeof(DEVMODE));
    }

    /* Configure new display mode */
    dm.dmPelsWidth = width;
    dm.dmPelsHeight = height;
    dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;

    if (refresh_rate > 0) {
        dm.dmDisplayFrequency = refresh_rate;
        dm.dmFields |= DM_DISPLAYFREQUENCY;
    }

    /* Try to change display settings */
    LONG result = ChangeDisplaySettings(&dm, CDS_FULLSCREEN);

    if (result == DISP_CHANGE_SUCCESSFUL) {
        fprintf(stderr, "Display mode changed to %dx%d @ %dHz\n",
                width, height, dm.dmDisplayFrequency);
        return true;
    } else {
        fprintf(stderr, "Failed to change display settings: %ld\n", result);
        return false;
    }
}

/**
 * Get the monitor for the current window
 */
static HMONITOR get_monitor_for_window(HWND hwnd) {
    if (!hwnd) {
        return NULL;
    }

    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    return monitor;
}

/**
 * Check if the monitor is the primary monitor
 */
static bool is_primary_monitor(HMONITOR monitor) {
    if (!monitor) {
        return true; /* Assume primary if no monitor info */
    }

    MONITORINFOEX mi;
    ZeroMemory(&mi, sizeof(mi));
    mi.cbSize = sizeof(mi);

    if (!GetMonitorInfo(monitor, (LPMONITORINFO)&mi)) {
        return true; /* Assume primary if we can't get info */
    }

    return (mi.dwFlags & MONITORINFOF_PRIMARY) != 0;
}

/**
 * Save original display settings
 */
static bool save_original_display_settings() {
    if (g_original_display_mode.dmSize != 0) {
        /* Already saved */
        return true;
    }

    DEVMODE dm;
    ZeroMemory(&dm, sizeof(dm));
    dm.dmSize = sizeof(dm);

    if (!EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm)) {
        fprintf(stderr, "Failed to enumerate display settings for saving\n");
        return false;
    }

    memcpy(&g_original_display_mode, &dm, sizeof(DEVMODE));
    return true;
}

/**
 * Restore display settings internally
 */
static bool restore_display_settings_internal() {
    if (g_original_display_mode.dmSize == 0) {
        /* No original settings to restore */
        return false;
    }

    /* Restore original display settings */
    LONG result = ChangeDisplaySettings(&g_original_display_mode, CDS_FULLSCREEN);

    if (result == DISP_CHANGE_SUCCESSFUL) {
        fprintf(stderr, "Display settings restored\n");
        /* Clear the saved settings after restoration */
        ZeroMemory(&g_original_display_mode, sizeof(DEVMODE));
        return true;
    } else {
        fprintf(stderr, "Failed to restore display settings: %ld\n", result);
        return false;
    }
}

#endif /* _WIN32 */
