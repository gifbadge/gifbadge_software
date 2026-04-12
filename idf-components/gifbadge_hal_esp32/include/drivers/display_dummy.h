/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once
#include <hal/display.h>

namespace hal::display::esp32s3 {
class display_dummy : public hal::display::Display {
  public:
    display_dummy();
    ~display_dummy() override = default;

    bool onColorTransDone(flushCallback_t) override;
    void write(int x_start, int y_start, int x_end, int y_end, void *color_data) override;
    void clear() override;
};
}

