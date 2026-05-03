/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once
#include <SDL3/SDL_events.h>

#include "debounce.h"
#include "hal/touch.h"

namespace hal::touch::oslinux {
class touch_sdl : public hal::touch::Touch {
  public:
    touch_sdl();
    touch_data read() override;
    void update(SDL_Event event);
  private:
    zmk_debounce_state _debounce_states;
    zmk_debounce_config _debounce_config = {0, 0};
    long last;
    std::pair<int16_t, int16_t> last_touch;
};
}

hal::touch::oslinux::touch_sdl *touch_sdl_init();

extern hal::touch::oslinux::touch_sdl *touchSdl;
