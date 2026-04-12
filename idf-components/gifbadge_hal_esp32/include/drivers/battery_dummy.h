/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once

#include <hal/battery.h>

namespace hal::battery::esp32s3 {
class battery_dummy final : public hal::battery::Battery {
 public:
  battery_dummy();
  ~battery_dummy() final = default;

  void poll();

  double BatteryVoltage() override;

  int BatterySoc() override;

  void BatteryRemoved() override;

  void BatteryInserted() override;

  State BatteryStatus() override;
};
}