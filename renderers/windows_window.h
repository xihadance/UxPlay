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

#ifndef WINDOWS_WINDOW_H
#define WINDOWS_WINDOW_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32

#include <windows.h>
#include <stdbool.h>

/**
 * Create an exclusive fullscreen window
 *
 * @param width Initial window width (used for display mode switching)
 * @param height Initial window height (used for display mode switching)
 * @param title Window title
 * @return Window handle (HWND), or NULL on failure
 */
HWND windows_create_fullscreen_window(int width, int height, const char *title);

/**
 * Get the current fullscreen window handle
 *
 * @return Current window handle, or NULL if no window exists
 */
HWND windows_get_window_handle();

/**
 * Check if we're in exclusive fullscreen mode
 *
 * @return true if in exclusive fullscreen mode, false otherwise
 */
bool windows_is_fullscreen_mode();

/**
 * Toggle between fullscreen and windowed mode
 *
 * @return true if successfully toggled, false otherwise
 */
bool windows_toggle_fullscreen();

/**
 * Restore display settings on exit
 *
 * This should be called before application exit to restore
 * the original display resolution
 */
void windows_restore_display_settings();

/**
 * Handle window messages (for fullscreen toggle)
 *
 * This function should be called from the main message loop
 * to handle fullscreen toggle events
 *
 * @param hwnd Window handle
 * @param msg Message ID
 * @param wParam Message parameter
 * @param lParam Message parameter
 * @return Result of message processing
 */
LRESULT windows_handle_message(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/**
 * Cleanup Windows window resources
 *
 * This should be called during application shutdown
 */
void windows_cleanup();

/**
 * Set the fullscreen toggle mode
 *
 * @param mode Fullscreen toggle mode
 *             0 = No toggle (always fullscreen or always windowed)
 *             1 = Alt+Enter toggle
 *             2 = F11 toggle
 */
void windows_set_toggle_mode(int mode);

#else

/* Non-Windows platforms - provide stub functions */

static inline HWND windows_create_fullscreen_window(int width, int height, const char *title) {
    return NULL;
}

static inline HWND windows_get_window_handle() {
    return NULL;
}

static inline bool windows_is_fullscreen_mode() {
    return false;
}

static inline bool windows_toggle_fullscreen() {
    return false;
}

static inline void windows_restore_display_settings() {
    /* Nothing to do on non-Windows platforms */
}

static inline void windows_cleanup() {
    /* Nothing to do on non-Windows platforms */
}

static inline void windows_set_toggle_mode(int mode) {
    /* Nothing to do on non-Windows platforms */
}

#endif /* _WIN32 */

#ifdef __cplusplus
}
#endif

#endif /* WINDOWS_WINDOW_H */
