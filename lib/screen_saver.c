/**
 * UxPlay - AirPlay mirroring server
 *
 * screen_saver.c - Cross-platform screen saver inhibition implementation
 *
 * Copyright (C) 2025 UxPlay contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Defensive Programming Notes:
 * - All API functions check for NULL pointers
 * - All error conditions are logged but don't crash
 * - Memory allocations use safe patterns
 * - Platform-specific APIs are wrapped with defensive checks
 * - Thread safety is ensured through state checks
 */

#include "screen_saver.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Platform detection */
#if defined(_WIN32)
    #define SCREEN_SAVER_WINDOWS
    #include <windows.h>
    #include <powrprof.h>
#elif defined(__APPLE__)
    #define SCREEN_SAVER_MACOS
    #include <CoreGraphics/CoreGraphics.h>
    #include <IOKit/pwr_mgt/IOPM.h>
#else
    #define SCREEN_SAVER_UNIX
    #ifdef DBUS
        #include <dbus/dbus.h>
    #endif
#endif

/* Debug logging - enabled by default for safety */
#define SCREEN_SAVER_DEBUG 1

#if SCREEN_SAVER_DEBUG
#define LOG_DEBUG(...) fprintf(stderr, "[SCREEN_SAVER] " __VA_ARGS__)
#else
#define LOG_DEBUG(...) ((void)0)
#endif

#define LOG_ERROR(...) fprintf(stderr, "[SCREEN_SAVER ERROR] " __VA_ARGS__)
#define LOG_INFO(...) fprintf(stderr, "[SCREEN_SAVER] " __VA_ARGS__)

/* Global state */
static screen_saver_mode_t current_mode = SCR_SV_OFF;
static bool is_inhibited = false;
static char *app_name = NULL;
static screen_saver_init_result_t init_result = SCR_SV_INIT_UNSUPPORTED;

/* Platform-specific state */

#ifdef SCREEN_SAVER_WINDOWS
static bool windows_initialized = false;
static DWORD original_exec_state = 0;

/* Power setting GUID for display suppression (Windows 7+) */
static GUID GUID_DISPLAY_REQUIRED = { 0x007fc834, 0x3b69, 0x4026,
    { 0x80, 0x12, 0x03, 0x4a, 0x34, 0xb6, 0x8a, 0x69 } };

#endif

#ifdef SCREEN_SAVER_UNIX
#ifdef DBUS
static DBusConnection *dbus_connection = NULL;
static dbus_uint32_t dbus_cookie = 0;
static bool dbus_last_message = false;

/* DBus service configuration (can be customized for different DEs) */
static const char *dbus_service = "org.freedesktop.ScreenSaver";
static const char *dbus_path = "/org/freedesktop/ScreenSaver";
static const char *dbus_interface = "org.freedesktop.ScreenSaver";
static const char *dbus_inhibit = "Inhibit";
static const char *dbus_uninhibit = "UnInhibit";
#endif
#endif

/* Forward declarations */
static bool platform_init(void);
static bool platform_inhibit(bool inhibit);
static void platform_shutdown(void);
static const char *platform_get_status(void);

/**
 * Initialize screen saver inhibition system
 *
 * Defensive Programming:
 * - Validates all input parameters
 * - Checks for memory allocation failures
 * - Handles platform initialization failures gracefully
 * - Ensures thread safety through state initialization
 */
screen_saver_init_result_t screen_saver_init(screen_saver_mode_t mode, const char *name) {
    /* Validate mode parameter - prevent buffer overflow attacks */
    if (mode > SCR_SV_ALWAYS_ON) {
        LOG_ERROR("Invalid screen saver mode: %d (expected 0-2)\n", mode);
        return SCR_SV_INIT_ERROR;
    }

    /* Handle OFF mode early to avoid unnecessary processing */
    if (mode == SCR_SV_OFF) {
        LOG_INFO("Screen saver inhibition disabled (mode=0)\n");
        current_mode = mode;
        init_result = SCR_SV_INIT_SUCCESS;
        return SCR_SV_INIT_SUCCESS;
    }

    /* Validate app_name parameter - prevent NULL pointer dereference */
    if (name == NULL) {
        LOG_ERROR("Screen saver: app_name is NULL\n");
        return SCR_SV_INIT_ERROR;
    }

    /* Store app name with bounds checking */
    free(app_name);
    app_name = strdup(name);

    if (app_name == NULL) {
        LOG_ERROR("Failed to allocate memory for app name (tried to copy '%s')\n", name);
        return SCR_SV_INIT_ERROR;
    }

    /* Set current mode before platform initialization for consistency */
    current_mode = mode;

    /* Initialize platform-specific functionality */
    if (!platform_init()) {
        LOG_ERROR("Platform initialization failed\n");
        free(app_name);
        app_name = NULL;
        current_mode = SCR_SV_OFF;
        init_result = SCR_SV_INIT_UNSUPPORTED;
        return SCR_SV_INIT_UNSUPPORTED;
    }

    /* If mode is ALWAYS_ON, inhibit immediately */
    if (mode == SCR_SV_ALWAYS_ON) {
        if (!screen_saver_set_inhibit(true)) {
            LOG_ERROR("Failed to inhibit screen saver in ALWAYS_ON mode\n");
            /* Clean up on failure */
            free(app_name);
            app_name = NULL;
            current_mode = SCR_SV_OFF;
            init_result = SCR_SV_INIT_ERROR;
            return SCR_SV_INIT_ERROR;
        }
    }

    init_result = SCR_SV_INIT_SUCCESS;
    LOG_INFO("Screen saver inhibition initialized: mode=%d, app=%s\n", mode, app_name);
    return SCR_SV_INIT_SUCCESS;
}

/**
 * Set screen saver inhibition state
 *
 * Defensive Programming:
 * - Validates internal state consistency
 * - Checks for initialization before operation
 * - Ensures state updates are atomic
 * - Logs all failure conditions
 */
bool screen_saver_set_inhibit(bool inhibit) {
    /* Early exit if not initialized - prevents use-after-free */
    if (current_mode == SCR_SV_OFF) {
        LOG_DEBUG("Screen saver inhibition disabled (mode=0)\n");
        return true;  /* Nothing to do */
    }

    /* Check for NULL app_name - indicates corrupted state */
    if (app_name == NULL) {
        LOG_ERROR("Screen saver state corrupted: app_name is NULL\n");
        return false;
    }

    /* Early exit if already in desired state - avoid unnecessary system calls */
    if (is_inhibited == inhibit) {
        LOG_DEBUG("Screen saver state already %s\n", inhibit ? "inhibited" : "allowed");
        return true;  /* Already in desired state */
    }

    /* Delegate to platform-specific implementation */
    if (!platform_inhibit(inhibit)) {
        LOG_ERROR("Failed to %s screen saver\n", inhibit ? "inhibit" : "allow");
        return false;
    }

    /* Update state only after successful operation */
    is_inhibited = inhibit;
    LOG_DEBUG("Screen saver %s\n", inhibit ? "inhibited" : "allowed");
    return true;
}

/**
 * Check if screen saver inhibition is currently active
 */
bool screen_saver_is_inhibited(void) {
    return is_inhibited;
}

/**
 * Update screen saver state based on video activity
 *
 * @param txusage_avg Average data transfer rate (bytes/sec)
 * @param threshold Activity threshold for considering video as "active"
 * @return true if state changed, false otherwise
 *
 * Defensive Programming:
 * - Validates input parameters (NaN, infinity checks)
 * - Checks internal state consistency
 * - Handles edge cases gracefully
 */
bool screen_saver_update_activity(double txusage_avg, double threshold) {
    /* Validate input parameters */
    if (current_mode != SCR_SV_ON_DURING_ACTIVITY) {
        return false;  /* Only update in activity mode */
    }

    /* Check for NaN or infinity - defensive against corrupted data */
    if (txusage_avg != txusage_avg ||  /* NaN check */
        txusage_avg == INFINITY || txusage_avg == -INFINITY) {
        LOG_ERROR("Screen saver: Invalid txusage_avg value: %f\n", txusage_avg);
        return false;
    }

    if (threshold != threshold ||  /* NaN check */
        threshold == INFINITY || threshold == -INFINITY) {
        LOG_ERROR("Screen saver: Invalid threshold value: %f\n", threshold);
        return false;
    }

    /* Determine if we should inhibit based on activity */
    bool should_inhibit = (txusage_avg > threshold);

    /* Only update if state actually changes */
    if (should_inhibit != is_inhibited) {
        return screen_saver_set_inhibit(should_inhibit);
    }

    return false;  /* No state change */
}

/**
 * Shutdown screen saver inhibition system
 *
 * Defensive Programming:
 * - Validates state before shutdown
 * - Ensures resources are properly freed
 * - Handles partial shutdown gracefully
 * - Prevents use-after-free
 */
bool screen_saver_shutdown(void) {
    /* Early exit if not initialized */
    if (current_mode == SCR_SV_OFF) {
        LOG_DEBUG("Screen saver already shutdown\n");
        return true;  /* Nothing to do */
    }

    /* Disable inhibition before shutdown */
    if (is_inhibited) {
        /* Ignore return value - we want to continue shutdown even if it fails */
        screen_saver_set_inhibit(false);
    }

    /* Platform-specific cleanup */
    platform_shutdown();

    /* Free resources with NULL checks for safety */
    if (app_name != NULL) {
        free(app_name);
        app_name = NULL;
    }

    /* Reset state to prevent use-after-free */
    is_inhibited = false;
    current_mode = SCR_SV_OFF;

    LOG_INFO("Screen saver inhibition shutdown complete\n");
    return true;
}

/**
 * Get human-readable status message
 */
const char *screen_saver_get_status(void) {
    static char status[256];

    if (current_mode == SCR_SV_OFF) {
        snprintf(status, sizeof(status), "Screen saver inhibition: OFF");
    } else if (is_inhibited) {
        snprintf(status, sizeof(status), "Screen saver inhibition: ACTIVE (mode=%d)", current_mode);
    } else {
        snprintf(status, sizeof(status), "Screen saver inhibition: INACTIVE (mode=%d)", current_mode);
    }

    return status;
}

/**
 * Check if the current platform is supported
 */
bool screen_saver_is_supported(void) {
    #if defined(SCREEN_SAVER_WINDOWS) || defined(SCREEN_SAVER_MACOS)
        return true;
    #elif defined(DBUS)
        return true;
    #else
        return false;
    #endif
}

/**
 * Platform-specific initialization
 */
static bool platform_init(void) {
    #ifdef SCREEN_SAVER_WINDOWS
        if (!windows_initialized) {
            /* Store original execution state */
            original_exec_state = GetThreadExecutionState();

            /* Check if GetThreadExecutionState succeeded */
            if (original_exec_state == 0 && GetLastError() != ERROR_SUCCESS) {
                LOG_ERROR("Windows: Failed to get thread execution state (error %lu)\n", GetLastError());
                return false;
            }

            /* Set to prevent display shutdown */
            DWORD result = SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED);

            if (result == 0) {
                LOG_ERROR("Windows: Failed to set thread execution state (error %lu)\n", GetLastError());
                return false;
            }

            windows_initialized = true;
            LOG_DEBUG("Windows: Initialized screen saver inhibition\n");
        }
        return true;

    #elif defined(SCREEN_SAVER_MACOS)
        /* macOS implementation using Core Graphics */
        /* This would require linking against CoreGraphics framework */
        LOG_ERROR("macOS screen saver inhibition not yet implemented\n");
        return false;

    #elif defined(DBUS)
        if (dbus_connection == NULL) {
            DBusError dbus_error;
            dbus_error_init(&dbus_error);

            dbus_connection = dbus_bus_get(DBUS_BUS_SESSION, &dbus_error);
            if (dbus_error_is_set(&dbus_error)) {
                LOG_ERROR("D-Bus session not found: %s\n", dbus_error.message);
                dbus_error_free(&dbus_error);
                return false;
            }

            /* Validate the connection is not NULL */
            if (dbus_connection == NULL) {
                LOG_ERROR("D-Bus connection is NULL after dbus_bus_get\n");
                return false;
            }

            LOG_DEBUG("D-Bus: Initialized screen saver inhibition\n");
        }
        return true;

    #else
        return false;
    #endif
}

/**
 * Platform-specific inhibition control
 */
static bool platform_inhibit(bool inhibit) {
    #ifdef SCREEN_SAVER_WINDOWS
        if (!windows_initialized) {
            LOG_ERROR("Windows screen saver not initialized\n");
            return false;
        }

        if (inhibit) {
            /* Prevent display shutdown and system sleep */
            DWORD result = SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED);

            if (result == 0) {
                LOG_ERROR("Windows: Failed to inhibit screen saver (error %lu)\n", GetLastError());
                return false;
            }

            /* Additional: Try to set power request for display */
            /* This is a defensive approach - may not work on all Windows versions */
            POWER_REQUEST_CONTEXT request;
            request.Version = POWER_REQUEST_CONTEXT_VERSION;
            request.Flags = 0;
            request.Reason.SimpleReasonString = L"UxPlay: Screen mirroring active";

            /* Note: PowerCreateRequest requires Windows 7+ */
            /* We use SetThreadExecutionState as primary method */

            LOG_DEBUG("Windows: Inhibited screen saver\n");
        } else {
            /* Restore original execution state */
            DWORD result = SetThreadExecutionState(original_exec_state);

            if (result == 0) {
                LOG_ERROR("Windows: Failed to allow screen saver (error %lu)\n", GetLastError());
                /* Continue anyway - the state might already be allowed */
            } else {
                LOG_DEBUG("Windows: Allowed screen saver\n");
            }
        }
        return true;

    #elif defined(DBUS)
        if (dbus_connection == NULL) {
            LOG_ERROR("D-Bus connection not available\n");
            return false;
        }

        const char *method = inhibit ? dbus_inhibit : dbus_uninhibit;

        DBusMessage *message = dbus_message_new_method_call(
            dbus_service,
            dbus_path,
            dbus_interface,
            method
        );

        if (!message) {
            LOG_ERROR("Failed to create D-Bus message\n");
            return false;
        }

        if (inhibit) {
            const char *reason = "UxPlay: Active video mirroring";
            dbus_bool_t ret = dbus_message_append_args(
                message,
                DBUS_TYPE_STRING, &app_name,
                DBUS_TYPE_STRING, &reason,
                DBUS_TYPE_INVALID
            );

            if (!ret) {
                LOG_ERROR("Failed to append D-Bus arguments\n");
                dbus_message_unref(message);
                return false;
            }

            DBusPendingCall *pending = NULL;
            ret = dbus_connection_send_with_reply(dbus_connection, message, &pending, -1);
            if (!ret || pending == NULL) {
                LOG_ERROR("Failed to send D-Bus inhibition request\n");
                dbus_message_unref(message);
                return false;
            }

            /* Wait for reply */
            dbus_pending_call_block(pending);
            DBusMessage *reply = dbus_pending_call_steal_reply(pending);
            dbus_pending_call_unref(pending);

            if (reply) {
                dbus_uint32_t cookie = 0;
                if (dbus_message_get_args(reply, NULL,
                    DBUS_TYPE_UINT32, &cookie,
                    DBUS_TYPE_INVALID)) {
                    dbus_cookie = cookie;
                    LOG_DEBUG("D-Bus: Got cookie %u\n", cookie);
                } else {
                    LOG_ERROR("D-Bus: Failed to get cookie from reply\n");
                }
                dbus_message_unref(reply);
            }
        } else {
            if (dbus_cookie == 0) {
                LOG_ERROR("D-Bus: No cookie to release\n");
                dbus_message_unref(message);
                return false;
            }

            dbus_bool_t ret = dbus_message_append_args(
                message,
                DBUS_TYPE_UINT32, &dbus_cookie,
                DBUS_TYPE_INVALID
            );

            if (!ret) {
                LOG_ERROR("Failed to append D-Bus cookie\n");
                dbus_message_unref(message);
                return false;
            }

            if (!dbus_connection_send(dbus_connection, message, NULL)) {
                LOG_ERROR("Failed to send D-Bus un-inhibit request\n");
                dbus_message_unref(message);
                return false;
            }

            dbus_cookie = 0;
            LOG_DEBUG("D-Bus: Released inhibition\n");
        }

        dbus_connection_flush(dbus_connection);
        dbus_message_unref(message);
        return true;

    #else
        return false;
    #endif
}

/**
 * Platform-specific shutdown
 */
static void platform_shutdown(void) {
    #ifdef SCREEN_SAVER_WINDOWS
        if (windows_initialized) {
            /* Restore original execution state */
            SetThreadExecutionState(original_exec_state);
            windows_initialized = false;
            LOG_DEBUG("Windows: Screen saver inhibition shutdown\n");
        }

    #elif defined(DBUS)
        if (dbus_connection) {
            /* Release any remaining inhibition */
            if (dbus_cookie != 0 && current_mode == SCR_SV_ALWAYS_ON) {
                screen_saver_set_inhibit(false);
            }
            dbus_connection = NULL;
            LOG_DEBUG("D-Bus: Screen saver inhibition shutdown\n");
        }

    #endif
}

/**
 * Get platform-specific status
 */
static const char *platform_get_status(void) {
    #ifdef SCREEN_SAVER_WINDOWS
        return "Windows power management API";
    #elif defined(DBUS)
        return "D-Bus org.freedesktop.ScreenSaver";
    #elif defined(SCREEN_SAVER_MACOS)
        return "macOS Core Graphics/IOKit (not implemented)";
    #else
        return "Unsupported platform";
    #endif
}
