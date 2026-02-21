/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include <SDL3/SDL_events.h>
#include "drivers/key_sdl.h"
#include "portable_time.h"
#include "log.h"
#include "hal/keys.h"
#include <array>

hal::keys::oslinux::keys_sdl *keysSdl;

hal::keys::oslinux::keys_sdl::keys_sdl() {
  _debounce_states[hal::keys::KEY_UP] = zmk_debounce_state{false, false, 0};
  _debounce_states[hal::keys::KEY_DOWN] = zmk_debounce_state{false, false, 0};
  _debounce_states[hal::keys::KEY_ENTER] = zmk_debounce_state{false, false, 0};
}

void hal::keys::oslinux::keys_sdl::poll() {
}
void hal::keys::oslinux::keys_sdl::update(SDL_Event event) {
  std::array<std::pair<SDL_Scancode, hal::keys::EVENT_CODE>, 3> keys = {{
    {SDL_SCANCODE_DOWN, hal::keys::KEY_DOWN},
    {SDL_SCANCODE_UP, hal::keys::KEY_UP},
    {SDL_SCANCODE_SPACE, hal::keys::KEY_ENTER},
}};
  auto time = millis();
  for (auto [scancode, eventcode] : keys) {
    if (event.key.scancode == scancode) {
      LOGI("key_sdl", "key %i", scancode);
      zmk_debounce_update(&_debounce_states[eventcode],
                        event.type == SDL_EVENT_KEY_DOWN,
                        static_cast<int>(time - last),
                        &_debounce_config);
    }
  }
  last = time;
}
hal::keys::EVENT_STATE *hal::keys::oslinux::keys_sdl::read() {
  for (int b = 0; b < KEY_MAX; b++) {
    hal::keys::EVENT_STATE state = zmk_debounce_is_pressed(&_debounce_states[b]) ? STATE_PRESSED : STATE_RELEASED;
    if (state == STATE_PRESSED && last_state[b] == state) {
      _currentState[b] = STATE_HELD;
    } else {
      last_state[b] = state;
      _currentState[b] = state;
    }
  }
  return _currentState;
}

hal::keys::oslinux::keys_sdl *keys_sdl_init() {
  keysSdl = new hal::keys::oslinux::keys_sdl();
  return keysSdl;
}
