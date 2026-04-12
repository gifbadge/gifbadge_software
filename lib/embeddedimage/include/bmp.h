/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once
#include "image.h"
#include "simplebmp.h"

namespace image {
class bmpImage: public Image {
  public:
    explicit bmpImage(screenResolution res, const char *path): Image(res, path) {};
    ~bmpImage() override;

    frameReturn GetFrame(uint8_t *outBuf, int16_t x, int16_t y) override;
    std::pair<int16_t, int16_t> Size() override;
    int Open(void *buffer) override;
    bool Animated() override;

    const char * GetLastError() override;
    static Image *Create(screenResolution res, const char *path);

  private:
  FILE *fp = nullptr;
  BMP _bmp = {};
  int _error = 0;
};
}
