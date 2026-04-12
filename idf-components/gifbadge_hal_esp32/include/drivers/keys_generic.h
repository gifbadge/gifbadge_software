/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once

#include "hal/keys.h"
#include "hal/gpio.h"
#include "debounce.h"

namespace hal::keys::esp32s3 {
class KeysGeneric: public hal::keys::Keys{
 public:
  KeysGeneric(gpio::Gpio *up, gpio::Gpio *down, gpio::Gpio *enter);
  EVENT_STATE *read() override;
  int pollInterval() override;
  void poll();

 private:
  gpio::Gpio *_keys[KEY_MAX]{};
  zmk_debounce_state _debounce_states[KEY_MAX]{};
  zmk_debounce_config _debounce_config = {10, 20};
  EVENT_STATE _currentState[KEY_MAX]{};
  long long last;
  esp_timer_handle_t keyTimer = nullptr;
};
}
