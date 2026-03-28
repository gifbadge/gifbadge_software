/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include "png.h"
#include <string>
#include "bitbank2.h"
#include "bmp.h"
#include "image.h"

#include "resize.h"
#include "simplebmp.h"

image::PNGImage::~PNGImage() {
    png.close();
}

std::pair<int16_t, int16_t> image::PNGImage::Size() {
    return {png.getWidth(), png.getHeight()};
}

image::Image *image::PNGImage::Create(screenResolution res, const char *path) {
    return new PNGImage(res, path);
}

typedef int32_t (*readfile)(PNGFILE *pFile, uint8_t *pBuf, int32_t iLen);
typedef int32_t (*seekfile)(PNGFILE *pFile, int32_t iPosition);


int image::PNGImage::Open(void *buffer) {
    _buffer = buffer;
    return png.open(_path, bb2OpenFile, bb2CloseFile, (readfile)bb2ReadFile, (seekfile)bb2SeekFile, PNGDraw);
}

int image::PNGImage::Open(uint8_t *bin, int size) {
    png.openRAM(bin, size, PNGDraw);
    return -1;
}

struct pngresize {
    PNG *png;
    Resize *resize;
    void *buffer;
} ;

int image::PNGImage::resize(uint8_t *outBuf, int16_t x_start, int16_t y_start, int16_t x, int16_t y) {
    png.close();
    png.open(_path, bb2OpenFile, bb2CloseFile, (readfile)bb2ReadFile, (seekfile)bb2SeekFile, PNGResize);
    Resize resize(png.getWidth(), png.getHeight(), x, y, reinterpret_cast<uint16_t *>(outBuf), static_cast<uint8_t *>(_buffer)+64*1024);
    decoded = true;
    pngresize config = {&png, &resize, _buffer};
    return png.decode((void *) &config, 0);
}
bool image::PNGImage::resizable() {
    if (_buffer) {
        return true;
    }
    return false;
}

image::frameReturn image::PNGImage::GetFrame(uint8_t *outBuf, int16_t x, int16_t y, int16_t width) {
    if (decoded) {
        png.close();
        png.open(_path, bb2OpenFile, bb2CloseFile, (readfile)bb2ReadFile, (seekfile)bb2SeekFile, PNGDraw);
    }
    decoded = true;
    pnguser config = {.png = &png, .buffer = outBuf, .x = x, .y = y, .width = width};
    png.decode((void *) &config, PNG_FAST_PALETTE);
    return {image::frameStatus::END, 0};
}

int image::PNGImage::PNGDraw(PNGDRAW *pDraw) {
    auto *config = (pnguser *) pDraw->pUser;
    auto *buffer = (uint16_t *) config->buffer;
    uint32_t y = (pDraw->y+config->y) * config->width;
    uint16_t *line = &buffer[y+config->x];
    config->png->getLineAsRGB565(pDraw, line, PNG_RGB565_LITTLE_ENDIAN, 0xffffffff);
    return 1;
}

int image::PNGImage::PNGResize(PNGDRAW *pDraw) {
    auto *config = static_cast<pngresize *>(pDraw->pUser);
    if (config->buffer) {
        config->png->getLineAsRGB565(pDraw, static_cast<uint16_t *>(config->buffer), PNG_RGB565_LITTLE_ENDIAN, 0xffffffff);
        config->resize->line(pDraw->y, static_cast<uint16_t *>(config->buffer));
    }
    return 1;
}

const char * image::PNGImage::GetLastError() {
    switch(png.getLastError()){
        case PNG_SUCCESS:
            return "PNG_SUCCESS";
        case PNG_INVALID_PARAMETER:
            return "PNG_INVALID_PARAMETER";
        case PNG_DECODE_ERROR:
            return "PNG_DECODE_ERROR";
        case PNG_MEM_ERROR:
            return "PNG_MEM_ERROR";
        case PNG_NO_BUFFER:
            return "PNG_NO_BUFFER";
        case PNG_UNSUPPORTED_FEATURE:
            return "PNG_UNSUPPORTED_FEATURE";
        case PNG_INVALID_FILE:
            return "PNG_INVALID_FILE";
        case PNG_TOO_BIG:
            return "PNG_TOO_BIG";
        default:
            return "Unknown";
    }
}
