/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include "jpeg.h"
#include <string>

#include "bitbank2.h"
#include "png.h"
#include "image.h"
#include "resize.h"
#include "simplebmp.h"

bool image::JPEG::resizable() {
  if (_buffer) {
    return true;
  }
  return false;
}

image::JPEG::~JPEG() {
  jpeg.close();
}

std::pair<int16_t, int16_t> image::JPEG::Size() {
    return {jpeg.getWidth(), jpeg.getHeight()};
}

image::Image *image::JPEG::Create(screenResolution res, const char *path) {
    return new image::JPEG(res, path);
}

int JPEGDraw(JPEGDRAW *pDraw){
  uint16_t *d;
  int y;
  auto *config = (pnguser *) pDraw->pUser;
  auto *buffer = (uint16_t *) config->buffer;
  for(int iY = 0; iY < pDraw->iHeight ; iY++){
    y = iY+pDraw->y + config->y; // current line
    d = &buffer[config->x + pDraw->x + (y * config->width)];
    memcpy(d, &pDraw->pPixels[iY*pDraw->iWidth], pDraw->iWidth * 2);
  }
return 1;
}


typedef int32_t (*readfile)(JPEGFILE *pFile, uint8_t *pBuf, int32_t iLen);
typedef int32_t (*seekfile)(JPEGFILE *pFile, int32_t iPosition);

int image::JPEG::Open(void *buffer) {
  _buffer = buffer;
  int ret = jpeg.open(_path, bb2OpenFile, bb2CloseFile, (readfile)bb2ReadFile, (seekfile)bb2SeekFile, JPEGDraw);
  jpeg.setPixelType(RGB565_LITTLE_ENDIAN);
  if (jpeg.getJPEGType() == JPEG_MODE_PROGRESSIVE) {
    // We don't want to support Progressive JPEGs. JPEGDec only supports reading their thumbnails
    _lastError = 255;
    return 1;
  }
  _lastError = jpeg.getJPEGType();
  return ret==0; //Invert the return value
}

image::frameReturn image::JPEG::GetFrame(uint8_t *outBuf, int16_t x, int16_t y, int16_t width) {
  if (decoded) {
    jpeg.close();
    jpeg.open(_path, bb2OpenFile, bb2CloseFile, (readfile)bb2ReadFile, (seekfile)bb2SeekFile, JPEGDraw);
    jpeg.setPixelType(RGB565_LITTLE_ENDIAN);
  }
  decoded = true;
  pnguser config = {.png = nullptr, .buffer = outBuf, .x = x, .y = y, .width = width};
  jpeg.setUserPointer(&config);
  jpeg.decode(0, 0, 0);
  _lastError = jpeg.getLastError();
  return {image::frameStatus::END, 0};
}

const char * image::JPEG::GetLastError() {
    switch(_lastError){
      case JPEG_SUCCESS:
        return "JPEG_SUCCESS";
      case JPEG_INVALID_PARAMETER:
        return "JPEG_INVALID_PARAMETER";
      case JPEG_DECODE_ERROR:
        return "JPEG_DECODE_ERROR";
      case JPEG_UNSUPPORTED_FEATURE:
        return "JPEG_UNSUPPORTED_FEATURE";
      case JPEG_INVALID_FILE:
        return "JPEG_INVALID_FILE";
      case 255:
        return "JPEG_PROGRESSIVE_NOT_SUPPORTED";
      default:
        return "UNKNOWN";
    };
}

struct jpgresize {
  uint16_t *buffer;
  int width;
  int height;
  Resize *resize;
  int y;
};

int JPEGResize(JPEGDRAW *pDraw){
  auto *config = static_cast<jpgresize *>(pDraw->pUser);
  auto *buffer = config->buffer;
  for(int iY = 0; iY < pDraw->iHeight ; iY++){
    uint16_t *d = &buffer[pDraw->x + (iY * config->width)];
    memcpy(d, &pDraw->pPixels[iY*pDraw->iWidth], pDraw->iWidth * 2);
  }
  if (pDraw->x + pDraw->iWidth == config->width) {
    for (int y = 0; y < pDraw->iHeight; y++) {
      config->resize->line(pDraw->y+y, &config->buffer[y*config->width]);
    }
  }
  return 1;
}

int image::JPEG::resize(uint8_t *outBuf, int16_t x_start, int16_t y_start, int16_t x, int16_t y) {
  jpeg.close();
  jpeg.open(_path, bb2OpenFile, bb2CloseFile, reinterpret_cast<readfile>(bb2ReadFile), reinterpret_cast<seekfile>(bb2SeekFile), JPEGResize);
  jpeg.setPixelType(RGB565_LITTLE_ENDIAN);
  memset(outBuf, 0, sizeof(x*y*2));

  Resize resize(jpeg.getWidth(), jpeg.getHeight(), x, y, reinterpret_cast<uint16_t *>(outBuf), static_cast<uint8_t *>(_buffer)+64*1024);
  decoded = true;
  jpgresize config = {static_cast<uint16_t *>(_buffer), jpeg.getWidth(), jpeg.getHeight(), &resize, 0};
  jpeg.setUserPointer(&config);
  return jpeg.decode(0, 0, 0)==0;
  }