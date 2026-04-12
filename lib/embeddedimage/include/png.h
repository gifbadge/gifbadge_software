/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once

#include "image.h"
#include <PNGdec.h>

struct pnguser {
    PNG *png;
    uint8_t *buffer;
    int16_t x;
    int16_t y;
    int16_t width;
};

namespace image {

class PNGImage : public image::Image {
    public:
    explicit PNGImage(screenResolution res, const char *path): Image(res, path) {};
    explicit PNGImage(screenResolution res): Image(res) {};

    ~PNGImage() override;

  frameReturn GetFrame(uint8_t *outBuf, int16_t x, int16_t y) override;

    std::pair<int16_t, int16_t> Size() override;

    static Image *Create(screenResolution res, const char *path);

    int Open(void *buffer) override;

    int Open(uint8_t *bin, int size);

    const char * GetLastError() override;
    bool resizable() override;
    int resize(uint8_t *outBuf, int16_t x_start, int16_t y_start, int16_t x, int16_t y) override;


protected:
    PNG png{};
    bool decoded = false;
    static int PNGDraw(PNGDRAW *pDraw);
    static int PNGResize(PNGDRAW *pDraw);
    void *_buffer = nullptr;
};
}

