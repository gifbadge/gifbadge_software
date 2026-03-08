/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once
#include "hal/keys.h"
#include "hal/board.h"

typedef void (*op)();

typedef struct {
  op press;
  op hold;
  uint32_t delay;
} keyCommands;

enum MAIN_STATES {
  MAIN_NONE,
  MAIN_NORMAL,
  MAIN_USB,
  MAIN_LOW_BATT,
  MAIN_OTA,
};


void initInputTimer(Boards::Board *board);
void startInputTimer();
void stopInputTimer();