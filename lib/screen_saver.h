/**
 * UxPlay - AirPlay mirroring server
 *
 * screen_saver.h - Cross-platform screen saver inhibition
 *
 * Provides unified API for inhibiting screen saver/sleep on different platforms:
 * - Windows: Uses SetThreadExecutionState and power management APIs
 * - Linux/*BSD: Uses D-Bus org.freedesktop.ScreenSaver interface
 * - macOS: Uses Core Graphics/IOKit (future)
 *
 * Copyright (C) 2025 UxPlay contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef SCREEN_SAVER_H
#define SCREEN_SAVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
 * Screen saver modes
 *
 * 0: OFF - No screen saver inhibition
 * 1: ON_DURING_ACTIVITY - Inhibit only during video activity
 * 2: ALWAYS_ON - Inhibit whenever UxPlay is running
 */
typedef enum {
    SCR_SV_OFF = 0,
    SCR_SV_ON_DURING_ACTIVITY = 1,
    SCR_SV_ALWAYS_ON = 2
} screen_saver_mode_t;

/**
 * Initialization result codes
 */
typedef enum {
    SCR_SV_INIT_SUCCESS = 0,
    SCR_SV_INIT_UNSUPPORTED = 1,
    SCR_SV_INIT_NO_SESSION = 2,
    SCR_SV_INIT_ERROR = 3
} screen_saver_init_result_t;

/**
 * Callback for video activity monitoring
 *
 * @param txusage_avg Average data transfer rate (bytes/sec)
 * @param threshold Activity threshold for considering video as "active"
 * @return true if inhibition should be active, false otherwise
 */
typedef bool (*screen_saver_activity_callback_t)(double txusage_avg, double threshold);

/**
 * Initialize screen saver inhibition system
 *
 * @param mode Screen saver mode (0=off, 1=on during activity, 2=always on)
 * @param app_name Application name for display in system dialog
 * @return Initialization result code
 */
screen_saver_init_result_t screen_saver_init(screen_saver_mode_t mode, const char *app_name);

/**
 * Set screen saver inhibition state
 *
 * @param inhibit true to inhibit screen saver, false to allow
 * @return true on success, false on failure
 */
bool screen_saver_set_inhibit(bool inhibit);

/**
 * Check if screen saver inhibition is currently active
 *
 * @return true if inhibition is active, false otherwise
 */
bool screen_saver_is_inhibited(void);

/**
 * Update screen saver state based on video activity
 *
 * @param txusage_avg Average data transfer rate (bytes/sec)
 * @param threshold Activity threshold for considering video as "active"
 * @return true if state changed, false otherwise
 */
bool screen_saver_update_activity(double txusage_avg, double threshold);

/**
 * Shutdown screen saver inhibition system
 *
 * @return true on success, false on failure
 */
bool screen_saver_shutdown(void);

/**
 * Get human-readable status message
 *
 * @return Status string (static buffer, do not free)
 */
const char *screen_saver_get_status(void);

/**
 * Check if the current platform is supported
 *
 * @return true if platform is supported, false otherwise
 */
bool screen_saver_is_supported(void);

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_SAVER_H */
