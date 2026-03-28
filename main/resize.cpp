/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include "resize.h"

#include <directory.h>
#include <esp_heap_caps.h>
#include <hw_init.h>
#include <JPEGENC.h>
#include <jpegenc.inl>
#include <log.h>
#include <sys/stat.h>
#include <utime.h>

static const char* TAG = "resize";

static int32_t jpeg_read(JPEGE_FILE *p, uint8_t *buffer, int32_t length) {
  const auto f = static_cast<FILE *>(p->fHandle);
  return static_cast<int32_t>(fread(buffer, 1,length, f));
}

static int32_t jpeg_write(JPEGE_FILE *p, uint8_t *buffer, int32_t length) {
  const auto f = static_cast<FILE *>(p->fHandle);
  return static_cast<int32_t>(fwrite(buffer, 1, length, f));
}

static int32_t jpeg_seek(JPEGE_FILE *p, int32_t position) {
  const auto f = static_cast<FILE *>(p->fHandle);
  fseek(f, position, SEEK_SET);
  return ftell(f);
}

int save_cache(const char *path, const char *cache_path, const uint8_t *buffer) {
#ifdef ESP_PLATFORM
  auto *_jpeg = static_cast<JPEGE_IMAGE *>(heap_caps_malloc(sizeof (JPEGE_IMAGE), MALLOC_CAP_SPIRAM));
#else
  auto *_jpeg = static_cast<JPEGE_IMAGE *>(malloc(sizeof (JPEGE_IMAGE)));
#endif
  char cache_dir[64] = "";
  strcpy(cache_dir, get_board()->GetStoragePath());
  strcat(cache_dir, "/.cache/");
  if (!is_directory(cache_dir)) {
    mkdir(cache_dir, S_IRWXU);
  }
  JPEGENCODE jpe;
  memset(_jpeg, 0, sizeof(JPEGE_IMAGE));
  _jpeg->pfnRead = jpeg_read;
  _jpeg->pfnWrite = jpeg_write;
  _jpeg->pfnSeek = jpeg_seek;
  _jpeg->JPEGFile.fHandle = fopen(cache_path, "w");
  _jpeg->pHighWater = &_jpeg->ucFileBuf[JPEGE_FILE_BUF_SIZE - 512];
  if (_jpeg->JPEGFile.fHandle == nullptr) {
    return 1;
  }
  const int iWidth = get_board()->GetDisplay()->size.first, iHeight = get_board()->GetDisplay()->size.second;
  if (int ret = JPEGEncodeBegin(_jpeg, &jpe, iWidth , iHeight, JPEGE_PIXEL_RGB565, JPEGE_SUBSAMPLE_444, JPEGE_Q_BEST); ret != JPEGE_SUCCESS) {
    LOGE(TAG, "JPEGEncodeBegin failed %d", ret);
    fclose(static_cast<FILE *>(_jpeg->JPEGFile.fHandle));
    free(_jpeg);
    return 1;
  }
  if (int ret = JPEGAddFrame(_jpeg, &jpe, const_cast<uint8_t *>(buffer) , iWidth*2); ret != JPEGE_SUCCESS) {
    LOGE(TAG, "JPEGAddFrame failed %d", ret);
    fclose(static_cast<FILE *>(_jpeg->JPEGFile.fHandle));
    free(_jpeg);
    return 1;
  }
  if (int ret = JPEGEncodeEnd(_jpeg); ret == 0) {
    LOGE(TAG, "JPEGEncodeEnd failed %d", ret);
    fclose(static_cast<FILE *>(_jpeg->JPEGFile.fHandle));
    free(_jpeg);
    return 1;
  }
  fclose(static_cast<FILE *>(_jpeg->JPEGFile.fHandle));
  free(_jpeg);

  struct stat f{};
  utimbuf new_times{};
  stat(path, &f);
  new_times.actime = f.st_atime; /* keep atime unchanged */
  new_times.modtime = f.st_mtime; /* set mtime to current time */
  utime(cache_path, &new_times);
  return 0;
}