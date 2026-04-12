/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include "gif.h"
#include <string>
#include "image.h"

#include <cstdio>
#include <sys/stat.h>
#include <cstring>

#include <AnimatedGIF.h>
#include <gif.inl>
#include <filebuffer.h>

char open_path[255] = "";

static void *OpenFile(const char *fname, int32_t *pSize) {
  strcpy(open_path, fname);
   FILE *infile = fopen(fname, "r");

   if (infile == nullptr) {
     printf("Couldn't open file %s\n", fname);
   }

   struct stat stats{};

   if (fstat(fileno(infile), &stats) != 0) {
     printf("Couldn't stat file\n");
     return nullptr;
   }

   *pSize = stats.st_size;
  fclose(infile);

  if (!filebuffer_open(fname)) {
    return nullptr;
  }
  return reinterpret_cast<void *>(1);
}

static void CloseFile(void *pHandle) {
  filebuffer_close();
}

static int32_t ReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen) {
  if (iLen <= 0) {
    return 0;
  }
  const int32_t ret = filebuffer_read(pBuf, iLen);
  pFile->iPos += ret;
  return ret;
}

static int32_t SeekFile(GIFFILE *pFile, int32_t iPosition) {
  filebuffer_seek(iPosition);
  pFile->iPos = iPosition;
  return 0;
}

image::GIF::GIF(screenResolution res, const char *path):Image(res, path) {};

image::GIF::~GIF() {
  if (_gif.pFrameBuffer != nullptr) {
    free(_gif.pFrameBuffer);
    _gif.pFrameBuffer = nullptr;
  }
  (*_gif.pfnClose)(_gif.GIFFile.fHandle);
}

image::frameReturn image::GIF::GetFrame(uint8_t *outBuf, int16_t x, int16_t y) {
  GIFUser gifuser = {outBuf, x, y, resolution.first};
  int frameDelay;
  if (_gif.GIFFile.iPos >= _gif.GIFFile.iSize - 1) // no more data exists
  {
    (*_gif.pfnSeek)(&_gif.GIFFile, 0); // seek to start
  }
  // int ret = playFrame(&frameDelay, &gifuser);
  if (GIFParseInfo(&_gif, 0)) {
    int rc;
    _gif.pUser = &gifuser;
    if (_gif.iError == GIF_EMPTY_FRAME) // don't try to decode it
      return {frameStatus::OK, frameDelay};
    if (_gif.pTurboBuffer) {
      rc = DecodeLZWTurbo(&_gif, 0);
    } else {
      rc = DecodeLZW(&_gif, 0);
    }
    if (rc != 0) // problem
      return {frameStatus::ERROR, 0};
  } else {
    // The file is "malformed" in that there is a bunch of non-image data after
    // the last frame. Return as if all is well, though if needed getLastError()
    // can be used to see if a frame was actually processed:
    // GIF_SUCCESS -> frame processed, GIF_EMPTY_FRAME -> no frame processed
    if (_gif.iError == GIF_EMPTY_FRAME) {
      return {frameStatus::END, 0};
    }
    return {frameStatus::ERROR, 0}; // error parsing the frame info, we may be at the end of the file
  }
  frameDelay = _gif.iFrameDelay;
  if (!_gif.pfnDraw) {
    if (x > 0 || y > 0) {
      // Handle cases where the GIF is smaller than the screen resolution
      auto *buffer = reinterpret_cast<uint16_t *>(outBuf);
      for (int i = 0; i < _gif.iCanvasHeight; i++) {
        int write_offset = resolution.second * (i + y) + x;
        int read_offset = (_gif.iCanvasWidth * _gif.iCanvasHeight) + (_gif.iCanvasWidth * i * 2);
        memcpy(&buffer[write_offset], &_gif.pFrameBuffer[read_offset], _gif.iCanvasWidth * 2);
      }
    } else {
      // GIF matches screen resolution
      memcpy(outBuf,
             &_gif.pFrameBuffer[resolution.first * resolution.second],
             resolution.first * resolution.second * 2);
    }
  }

  if (_gif.GIFFile.iPos < _gif.GIFFile.iSize - 10 == false) {
    return {frameStatus::END, frameDelay};
  }
  return {frameStatus::OK, frameDelay};
}

std::pair<int16_t, int16_t> image::GIF::Size() {
  return {_gif.iCanvasWidth, _gif.iCanvasHeight};
}

void image::GIF::GIFDraw(GIFDRAW *pDraw) {
  auto *gifuser = static_cast<GIFUser *>(pDraw->pUser);
  auto *buffer = reinterpret_cast<uint16_t *>(gifuser->buffer);

  int y = pDraw->iY + pDraw->y + gifuser->y; // current line
  uint16_t *d = &buffer[gifuser->x + pDraw->iX + (y * gifuser->width)];
  memcpy(d, pDraw->pPixels, pDraw->iWidth * 2);
}

image::Image *image::GIF::Create(screenResolution res, const char *path) {
  return new image::GIF(res, path);
}

int image::GIF::Open(void *buffer) {
  unsigned char ucPaletteType = LITTLE_ENDIAN_PIXELS;


  memset(&_gif, 0, sizeof(_gif));
  if (ucPaletteType != GIF_PALETTE_RGB565_LE && ucPaletteType != GIF_PALETTE_RGB565_BE
      && ucPaletteType != GIF_PALETTE_RGB888)
    _gif.iError = GIF_INVALID_PARAMETER;
  _gif.ucPaletteType = ucPaletteType;
  _gif.ucDrawType = GIF_DRAW_RAW; // assume RAW pixel handling
  _gif.pFrameBuffer = nullptr;

  _gif.iError = GIF_SUCCESS;
  _gif.pfnRead = ReadFile;
  _gif.pfnSeek = SeekFile;
  _gif.pfnDraw = nullptr;
  _gif.pfnOpen = OpenFile;
  _gif.pfnClose = CloseFile;
  _gif.GIFFile.fHandle = (*_gif.pfnOpen)(_path, &_gif.GIFFile.iSize);
  if (_gif.GIFFile.fHandle == nullptr) {
    _gif.iError = GIF_FILE_NOT_OPEN;
    return -1;
  }

  if (GIFInit(&_gif)) {
    if (buffer) {
      _gif.pTurboBuffer = static_cast<uint8_t *>(buffer);
    }
    else {
      printf("Turbo Buffer is null\n");
    }

    _gif.pFrameBuffer = static_cast<unsigned char *>(malloc(_gif.iCanvasWidth * _gif.iCanvasHeight * 3));
    if (_gif.pFrameBuffer == nullptr) {
      printf("Malloc full sized buffer failed. Falling back to row buffer\n");
      _gif.pFrameBuffer = static_cast<unsigned char *>(malloc(_gif.iCanvasWidth * (_gif.iCanvasHeight + 3)));
      _gif.pfnDraw = GIFDraw;
    }
    if (_gif.pFrameBuffer == nullptr) {
      _gif.iError = GIF_ERROR_MEMORY;
      return -1;
    }
    _gif.ucDrawType = GIF_DRAW_COOKED;
    return 0;
  }
  return -1;
}

const char *image::GIF::GetLastError() {
  switch (_gif.iError) {
    case GIF_SUCCESS:
      return "GIF_SUCCESS";
    case GIF_DECODE_ERROR:
      return "GIF_DECODE_ERROR";
    case GIF_TOO_WIDE:
      return "GIF_TOO_WIDE";
    case GIF_INVALID_PARAMETER:
      return "GIF_INVALID_PARAMETER";
    case GIF_UNSUPPORTED_FEATURE:
      return "GIF_UNSUPPORTED_FEATURE";
    case GIF_FILE_NOT_OPEN:
      return "GIF_FILE_NOT_OPEN";
    case GIF_EARLY_EOF:
      return "GIF_EARLY_EOF";
    case GIF_EMPTY_FRAME:
      return "GIF_EMPTY_FRAME";
    case GIF_BAD_FILE:
      return "GIF_BAD_FILE";
    case GIF_ERROR_MEMORY:
      return "GIF_ERROR_MEMORY";
    default:
      return "Unknown";
  }
}
