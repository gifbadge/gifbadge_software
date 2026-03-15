/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once

#include <cstdint>

namespace hal::gpio {
enum class GpioPullMode {
  NONE,
  UP,
  DOWN
};
enum class GpioDirection {
  IN,
  OUT
};

enum class GpioIntDirection {
  NONE,
  RISING,
  FALLING,
};

enum class GpioState {
  INVALID,
  LOW,
  HIGH
};

class Gpio{
 public:
  Gpio() = default;
  virtual void GpioConfig(GpioDirection direction, GpioPullMode pull) = 0;
  virtual GpioState GpioRead() = 0;
  virtual void GpioWrite(bool b) = 0;
  virtual void GpioInt(hal::gpio::GpioIntDirection dir, void (*callback)()) {};
};
}
