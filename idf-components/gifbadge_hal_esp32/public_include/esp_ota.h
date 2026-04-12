/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once
#include <stdint.h>
typedef struct {
  uint8_t num_supported_boards;
  uint8_t supported_boards[255];
} esp_custom_app_desc_t;
