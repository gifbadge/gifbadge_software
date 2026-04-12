/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once

#include <cstdint>
#include <sys/types.h>

#include <JPEGDEC.h>

#include "image.h"

namespace image {

class JPEG : public image::Image {
  public:

    JPEG(screenResolution res, const char *path): Image(res, path) {};

    ~JPEG() override;

  frameReturn GetFrame(uint8_t *outBuf, int16_t x, int16_t y) override;

    std::pair<int16_t, int16_t> Size() override;

    static Image* Create(screenResolution res, const char *path);

    int Open(void *buffer) override;

    const char * GetLastError() override;

    bool resizable() override;

    int resize(uint8_t *outBuf, int16_t x_start, int16_t y_start, int16_t x, int16_t y) override;

private:
    JPEGDEC jpeg;
    bool decoded = false;
    int _lastError = 0;
    void *_buffer;
};
}

