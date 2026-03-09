/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once

#include <queue.h>
#include <string>
#include <vector>

#include "FreeRTOS.h"
#include "task.h"

#include "hal/backlight.h"
#include "hal/display.h"

#define noResetBit (1<<5)

enum DISPLAY_OPTIONS {
  DISPLAY_NONE = 0,
  DISPLAY_FILE = 1,
  DISPLAY_NEXT = 2 | noResetBit,
  DISPLAY_PREVIOUS = 3 | noResetBit,
  DISPLAY_BATT = 4,
  DISPLAY_SPECIAL_1 = 7,
  DISPLAY_SPECIAL_2 = 8,
  DISPLAY_NOTIFY_CHANGE = 9,
  DISPLAY_NOTIFY_USB = 10,
  DISPLAY_ADVANCE = 11| noResetBit,
  DISPLAY_STOP = 12,
  DISPLAY_MENU = 13,
};

void display_task(void *params);

extern float last_fps;