/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include <esp_timer.h>
#include "log.h"
#include "drivers/keys_esp_io_expander.h"

static const char *TAG = "keys_esp_io_expander";

hal::keys::esp32s3::keys_esp_io_expander::keys_esp_io_expander(esp_io_expander_handle_t io_expander, const int up, const int down, const int enter)
    : _io_expander(io_expander) {

  buttonConfig[KEY_UP] = up;
  buttonConfig[KEY_DOWN] = down;
  buttonConfig[KEY_ENTER] = enter;

  _debounce_states[KEY_UP] = zmk_debounce_state{false, false, 0};
  _debounce_states[KEY_DOWN] = zmk_debounce_state{false, false, 0};
  _debounce_states[KEY_ENTER] = zmk_debounce_state{false, false, 0};

  last = esp_timer_get_time() / 1000;
}

hal::keys::EVENT_STATE * hal::keys::esp32s3::keys_esp_io_expander::read() {
  for (int b = 0; b < KEY_MAX; b++) {
    if (buttonConfig[b] >= 0) {
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

void hal::keys::esp32s3::keys_esp_io_expander::poll() {
  uint32_t levels = lastLevels;
  if (esp_io_expander_get_level(_io_expander, 0xffff, &levels) == ESP_OK) {
    //only update when we have a good read
    lastLevels = levels;

    const auto time = esp_timer_get_time() / 1000;

    for (int b = 0; b < KEY_MAX; b++) {
      if (buttonConfig[b] >= 0) {
        const bool state = levels & (1 << buttonConfig[b]);
        zmk_debounce_update(&_debounce_states[b], state == 0, static_cast<int>(time - last), &_debounce_config);
        if (zmk_debounce_get_changed(&_debounce_states[b])) {
          LOGI(TAG, "%i changed", b);
        }
      }
    }
    last = time;
  }
}

