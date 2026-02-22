/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once

#include "console.h"
#define DEBUG 0
#define LOGI(tag, format, ...) console_print(tag, format, ##__VA_ARGS__)
#define LOGD(tag, format, ...) do { if (DEBUG) console_print(tag, format, ##__VA_ARGS__); } while (0)
