/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once

#include "FreeRTOS.h"
#include "task.h"
#include <semphr.h>

#ifndef ESP_PLATFORM
#include "timers.h"
#endif


#include "log.h"
#include <lvgl.h>
#include "hal/config_storage.h"
#include "hal/board.h"
#include "display.h"

#define LVGL_TICK_PERIOD_MS    5
#define LVGL_TASK_MAX_DELAY_MS 500
#define LVGL_TASK_MIN_DELAY_MS 1
#define LVGL_TASK_PRIORITY     2

struct flushCbData {
  hal::display::Display *display;
  bool callbackEnabled;
};

extern "C" {
extern lv_indev_t *lvgl_encoder;
extern lv_indev_t *lvgl_touch;
}

enum LVGL_TASK_SIGNALS {
  LVGL_NONE,
  LVGL_STOP,
  LVGL_EXIT,
  LVGL_RESUME_MENU,
  LVGL_RESUME_USB
};

void lvgl_init(Boards::Board *);
void lvgl_menu_open();
bool lvgl_menu_state();
void lvgl_usb_open();

lv_obj_t *create_screen();
void destroy_screens();

typedef lv_obj_t *(*MenuType)();

#ifdef ESP_PLATFORM
IRAM_ATTR void lv_tick(TimerHandle_t);
#else
void lv_tick(TimerHandle_t);
#endif


