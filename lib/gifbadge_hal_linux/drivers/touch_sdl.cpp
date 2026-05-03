/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include "drivers/touch_sdl.h"

#include "log.h"
#include "portable_time.h"

hal::touch::oslinux::touch_sdl *touchSdl;

hal::touch::oslinux::touch_sdl::touch_sdl() {
  _debounce_states = zmk_debounce_state{false, false, 0};
}
hal::touch::touch_data hal::touch::oslinux::touch_sdl::read() {
  if (zmk_debounce_is_pressed(&_debounce_states)) {
    return {TOUCH_PRESS, last_touch};
  }
  return {TOUCH_RELEASE, {-1, -1}};
}
void hal::touch::oslinux::touch_sdl::update(SDL_Event event) {
  if (event.type == SDL_EVENT_MOUSE_MOTION) {
    // LOGI("touch_sdl", "SDL_EVENT_MOUSE_MOTION: x: %f y: %f\n", event.motion.x, event.motion.y);
    last_touch = std::make_pair<int16_t, int16_t>(event.motion.x, event.motion.y);
  }
  else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
    // LOGI("touch_sdl", "SDL_EVENT_MOUSE_BUTTON: button: %d down: %d\n", event.button.button, event.button.down);
    auto time = millis();
    zmk_debounce_update(&_debounce_states,
                         event.button.down,
                        static_cast<int>(time - last),
                        &_debounce_config);
    last = time;
  }
}

hal::touch::oslinux::touch_sdl *touch_sdl_init() {
  touchSdl = new hal::touch::oslinux::touch_sdl();
  return touchSdl;
}