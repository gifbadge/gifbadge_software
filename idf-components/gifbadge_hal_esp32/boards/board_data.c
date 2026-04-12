/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include "esp_ota.h"
#include "boards/boards.h"
#include "sdkconfig.h"

#if CONFIG_SPIRAM_MODE_OCT
const __attribute__((section(".rodata_custom_desc"))) esp_custom_app_desc_t custom_app_desc = {4, {BOARD_2_1_V0_2, BOARD_2_1_V0_4, BOARD_2_1_V0_6, BOARD_2_1_V0_7}};
#elif CONFIG_SPIRAM_MODE_QUAD
const __attribute__((section(".rodata_custom_desc"))) esp_custom_app_desc_t custom_app_desc = {3, {BOARD_1_28_V0, BOARD_1_28_V0_1, BOARD_1_28_V0_3}};
#endif