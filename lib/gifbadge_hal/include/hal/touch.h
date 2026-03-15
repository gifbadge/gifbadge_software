/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once

#include <cstdint>
#include <utility>

namespace hal::touch {
enum TOUCH_STATE {
  TOUCH_INVALID = 0,
  TOUCH_PRESS,
  TOUCH_RELEASE,
};

struct touch_data {
  TOUCH_STATE state;
  std::pair<int16_t, int16_t> position;
};
class Touch {
 public:
  Touch() = default;

  virtual ~Touch() = default;

  virtual touch_data read() = 0;
};
}

