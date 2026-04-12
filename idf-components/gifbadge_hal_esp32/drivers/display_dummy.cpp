/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include "drivers/display_dummy.h"

#include <esp_heap_caps.h>

hal::display::esp32s3::display_dummy::display_dummy() {
  buffer = static_cast<uint8_t *>(heap_caps_malloc(480 * 480 * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  size = {480, 480};
}
bool hal::display::esp32s3::display_dummy::onColorTransDone(flushCallback_t) {
  return true;
}
void hal::display::esp32s3::display_dummy::write(int x_start, int y_start, int x_end, int y_end, void *color_data) {
}
void hal::display::esp32s3::display_dummy::clear() {
}
