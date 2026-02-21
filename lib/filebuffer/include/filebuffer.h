/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once
#include <cstdint>
#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif
  enum FILEBUFFER_OPTIONS {
    FILEBUFFER_NONE,
    FILEBUFFER_CLOSE,
    FILEBUFFER_PAUSE,
    FILEBUFFER_STOP
  };
  void FileBufferTask(void *);
  bool filebuffer_open(const char *path);
  void filebuffer_close();
  int32_t filebuffer_read(uint8_t *pBuf, int32_t iLen);
  void filebuffer_seek(int32_t pos);

#ifdef __cplusplus
}
#endif