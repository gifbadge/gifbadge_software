/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once

#include <AnimatedGIF.h>
#include "image.h"

struct GIFUser{
    uint16_t *buffer;
  int16_t x;
  int16_t y;
    int32_t width;
};

namespace image {
class GIF : public Image {

public:
    explicit GIF(screenResolution res, const char *path);

    ~GIF() override;

    int Open(void *buffer) override;

  frameReturn GetFrame(uint8_t *outBuf, int16_t offsetX, int16_t offsetY) override;

    std::pair<int16_t, int16_t> Size() final;

    static Image * Create(screenResolution res, const char *path);

    bool Animated() override {return true;};

    const char *GetLastError() override;


private:
    GIFIMAGE _gif{};

  static void GIFDraw(GIFDRAW *pDraw);
};
}

