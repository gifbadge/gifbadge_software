/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include "drivers/backlight_dummy.h"

hal::backlight::esp32s3::backlight_dummy::backlight_dummy() {

}
void hal::backlight::esp32s3::backlight_dummy::state(bool) {

}
void hal::backlight::esp32s3::backlight_dummy::setLevel(int level) {
  lastLevel = level;
}
int hal::backlight::esp32s3::backlight_dummy::getLevel() {
  return lastLevel;
}
