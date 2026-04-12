/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include <esp_timer.h>
#include "drivers/keys_generic.h"
#include "log.h"

static const char *TAG = "keys_generic";

hal::keys::EVENT_STATE *hal::keys::esp32s3::KeysGeneric::read() {
  for (int b = 0; b < KEY_MAX; b++) {
    if (_keys[b] != nullptr) {
      EVENT_STATE state = zmk_debounce_is_pressed(&_debounce_states[b]) ? STATE_PRESSED : STATE_RELEASED;
      if (state == STATE_PRESSED && last_state[b] == state) {
        _currentState[b] = STATE_HELD;
      } else {
        last_state[b] = state;
        _currentState[b] = state;
      }
    }
  }
  return _currentState;
}

int hal::keys::esp32s3::KeysGeneric::pollInterval() {
  return 0;
}

hal::keys::esp32s3::KeysGeneric::KeysGeneric(gpio::Gpio *up, gpio::Gpio *down, gpio::Gpio *enter) {
  _keys[KEY_UP] = up,
      _keys[KEY_DOWN] = down,
      _keys[KEY_ENTER] = enter;

  _debounce_states[KEY_UP] = zmk_debounce_state{false, false, 0};
  _debounce_states[KEY_DOWN] = zmk_debounce_state{false, false, 0};
  _debounce_states[KEY_ENTER] = zmk_debounce_state{false, false, 0};

  last = esp_timer_get_time() / 1000;
}
void hal::keys::esp32s3::KeysGeneric::poll() {
  const auto time = esp_timer_get_time() / 1000;

  for (int b = 0; b < KEY_MAX; b++) {
    if (_keys[b] != nullptr) {
      gpio::GpioState state = _keys[b]->GpioRead();
      if (state != gpio::GpioState::INVALID) {
        zmk_debounce_update(&_debounce_states[b], state == gpio::GpioState::LOW, static_cast<int>(time - last), &_debounce_config);
        if (zmk_debounce_get_changed(&_debounce_states[b])) {
          LOGI(TAG, "%i changed", b);
        }
      }
    }
  }
  last = time;
}
