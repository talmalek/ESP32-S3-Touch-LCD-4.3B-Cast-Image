#pragma once

#include "sdkconfig.h"
#ifdef CONFIG_ARDUINO_RUNNING_CORE
#include <Arduino.h>
#endif
#include "esp_display_panel.hpp"
#include "lvgl.h"

#define LVGL_PORT_TICK_PERIOD_MS                (2)
#define LVGL_PORT_BUFFER_MALLOC_CAPS            (MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
#define LVGL_PORT_BUFFER_SIZE_HEIGHT            (64)
#define LVGL_PORT_BUFFER_NUM                    (2)
#define LVGL_PORT_TASK_MAX_DELAY_MS             (500)
#define LVGL_PORT_TASK_MIN_DELAY_MS             (2)
#define LVGL_PORT_TASK_STACK_SIZE               (6 * 1024)
#define LVGL_PORT_TASK_PRIORITY                 (2)
#define LVGL_PORT_TASK_CORE                     (1)

/**
 * Avoid tearing mode is DISABLED for this board.
 * The avoid-tear flush callback uses ulTaskNotifyTake(portMAX_DELAY) which
 * deadlocks because the vsync notification from the RGB LCD ISR doesn't
 * reliably reach the LVGL task on this hardware configuration.
 */
#ifdef LVGL_PORT_AVOID_TEARING_MODE
#undef LVGL_PORT_AVOID_TEARING_MODE
#endif
#define LVGL_PORT_AVOID_TEARING_MODE            (0)

#if LVGL_PORT_AVOID_TEARING_MODE != 0

#ifndef LVGL_PORT_ROTATION_DEGREE
#define LVGL_PORT_ROTATION_DEGREE               (0)
#endif

#define LVGL_PORT_AVOID_TEAR                    (1)

#if LVGL_PORT_AVOID_TEARING_MODE == 1
    #define LVGL_PORT_DISP_BUFFER_NUM           (2)
    #define LVGL_PORT_FULL_REFRESH              (1)
#elif LVGL_PORT_AVOID_TEARING_MODE == 2
    #define LVGL_PORT_DISP_BUFFER_NUM           (3)
    #define LVGL_PORT_FULL_REFRESH              (1)
#elif LVGL_PORT_AVOID_TEARING_MODE == 3
    #define LVGL_PORT_DISP_BUFFER_NUM           (2)
    #define LVGL_PORT_DIRECT_MODE               (1)
#else
    #error "Invalid avoid tearing mode"
#endif

#if (LVGL_PORT_ROTATION_DEGREE != 0) && (LVGL_PORT_ROTATION_DEGREE != 90) && \
    (LVGL_PORT_ROTATION_DEGREE != 180) && (LVGL_PORT_ROTATION_DEGREE != 270)
    #error "Invalid rotation degree, please set to 0, 90, 180 or 270"
#elif LVGL_PORT_ROTATION_DEGREE != 0
    #ifdef LVGL_PORT_DISP_BUFFER_NUM
        #undef LVGL_PORT_DISP_BUFFER_NUM
        #define LVGL_PORT_DISP_BUFFER_NUM       (3)
    #endif
#endif

#endif /* LVGL_PORT_AVOID_TEARING_MODE */

#ifdef __cplusplus
extern "C" {
#endif

bool lvgl_port_init(esp_panel::drivers::LCD *lcd, esp_panel::drivers::Touch *tp);
bool lvgl_port_lock(int timeout_ms);
bool lvgl_port_unlock(void);
void lvgl_port_stop(void);
void lvgl_port_resume(void);

#ifdef __cplusplus
}
#endif