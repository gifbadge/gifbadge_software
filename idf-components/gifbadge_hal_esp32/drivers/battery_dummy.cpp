/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include "drivers/battery_dummy.h"
#include "hal/battery.h"
hal::battery::esp32s3::battery_dummy::battery_dummy() = default;
void hal::battery::esp32s3::battery_dummy::battery_dummy::poll() {

}
double hal::battery::esp32s3::battery_dummy::BatteryVoltage() {
  return 4.2;
}
int hal::battery::esp32s3::battery_dummy::BatterySoc() {
  return 100;
}
void hal::battery::esp32s3::battery_dummy::BatteryRemoved() {

}
void hal::battery::esp32s3::battery_dummy::BatteryInserted() {

}
hal::battery::Battery::State hal::battery::esp32s3::battery_dummy::BatteryStatus() {
  return hal::battery::Battery::State::OK;
}
