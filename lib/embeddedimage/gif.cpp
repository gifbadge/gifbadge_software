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

image::GIF::GIF(screenResolution res, const char *path):Image(res, path) {}

image::GIF::~GIF() {
  if (_gif.pFrameBuffer != nullptr) {
    free(_gif.pFrameBuffer);
    _gif.pFrameBuffer = nullptr;
  }
  (*_gif.pfnClose)(_gif.GIFFile.fHandle);
}

#if defined (ARDUINO_ESP32S3_DEV) && !defined(NO_SIMD)
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
  void s3_palette_buffer(uint8_t *pSrc, uint8_t *pRGB565, uint16_t *pTransparent, int iLen, uint32_t *pPalette);
#ifdef __cplusplus
}
#endif // __cplusplus
#endif // ESP32S3 SIMD

uint32_t __attribute__((aligned(16))) u32Palette[256]; //This seems to perform better when not on the stack.

image::frameReturn image::GIF::GetFrame(uint8_t *outBuf, int16_t offsetX, int16_t offsetY) {
  GIFUser gifuser = {reinterpret_cast<uint16_t *>(outBuf), offsetX, offsetY, resolution.first};
  int frameDelay;
  if (_gif.GIFFile.iPos >= _gif.GIFFile.iSize - 1) // no more data exists
  {
    (*_gif.pfnSeek)(&_gif.GIFFile, 0); // seek to start
  }
  if (GIFParseInfo(&_gif, 0)) {
    int rc;
    if (_gif.iError == GIF_EMPTY_FRAME) // don't try to decode it
      return {frameStatus::OK, frameDelay};
    if (_gif.pTurboBuffer) {
      rc = DecodeLZWTurbo(&_gif, 0);
    } else {
      _gif.pUser = &gifuser;
      rc = DecodeLZW(&_gif, 0);
      _gif.pUser = nullptr;
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
    uint16_t *pPalette = (_gif.bUseLocalPalette) ? _gif.pLocalPalette : _gif.pPalette;
    uint16_t transparent = _gif.ucTransparent;
    uint8_t ucHasTransparency = _gif.ucGIFBits & 1;
    uint16_t *pTransparent = ucHasTransparency ? &transparent : nullptr;

    for (int x = 0; x < 256; x++) {
      u32Palette[x] = pPalette[x]; // expand 16->32bit entries for SIMD code
    }
    for (int y = 0; y < _gif.iHeight; y++) {
      int localY = y;
      // Ugly logic to handle the interlaced line position, but it
      // saves having to have another set of state variables
      if (_gif.ucMap & 0x40) {
        // interlaced?
        int height = _gif.iHeight - 1;
        if (localY > height / 2)
          localY = localY * 2 - (height | 1);
        else if (localY > height / 4)
          localY = localY * 4 - ((height & ~1) | 2);
        else if (localY > height / 8)
          localY = localY * 8 - ((height & ~3) | 4);
        else
          localY = localY * 8;
      }
      void *pBuf = _gif.pTurboBuffer+_gif.iCanvasWidth * _gif.iCanvasHeight;
      std::size_t sz = TURBO_BUFFER_SIZE;
      auto *pAligned = static_cast<uint8_t *>(std::align(16, _gif.iWidth, pBuf, sz));
      memcpy(pAligned, &_gif.pTurboBuffer[(localY * _gif.iWidth)], _gif.iWidth);//Prevents Tearing like effect. Assuming that's memory alignment issues.
      uint8_t *fb = _gif.pFrameBuffer;
      fb += 2 * (_gif.iX+offsetX) + (localY + _gif.iY+offsetY) * (resolution.first * 2);
      s3_palette_buffer(pAligned, fb, pTransparent, _gif.iWidth, u32Palette);
    }
      memcpy(outBuf, _gif.pFrameBuffer, resolution.first * resolution.second * 2);
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
  auto *buffer = gifuser->buffer;

  int y = pDraw->iY + pDraw->y + gifuser->y; // current line
  uint16_t *d = &buffer[gifuser->x + pDraw->iX + (y * gifuser->width)];
  memcpy(d, pDraw->pPixels, pDraw->iWidth * 2);
}

image::Image *image::GIF::Create(screenResolution res, const char *path) {
  return new GIF(res, path);
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
    _gif.ucDrawType = GIF_DRAW_COOKED;
    if (buffer) {
      _gif.pTurboBuffer = static_cast<uint8_t *>(buffer);
      _gif.pFrameBuffer = static_cast<unsigned char *>(aligned_alloc(16, resolution.first * resolution.second * 2));
      _gif.ucDrawType = GIF_DRAW_RAW;
    }
    else {
      _gif.pFrameBuffer = static_cast<unsigned char *>(aligned_alloc(16, resolution.first * resolution.second * 3));
      printf("Turbo Buffer is null\n");
    }
    if (_gif.pFrameBuffer == nullptr) {
      printf("Malloc full sized buffer failed. Falling back to row buffer\n");
      _gif.pFrameBuffer = static_cast<unsigned char *>(malloc(_gif.iCanvasWidth * (_gif.iCanvasHeight + 3)));
      _gif.pfnDraw = GIFDraw;
      _gif.ucDrawType = GIF_DRAW_COOKED;
    }
    if (_gif.pFrameBuffer == nullptr) {
      _gif.iError = GIF_ERROR_MEMORY;
      return -1;
    }
    memset(_gif.pFrameBuffer, 0x0, resolution.first*resolution.second*2);
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
