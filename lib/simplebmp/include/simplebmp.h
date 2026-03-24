/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once
#include <cstdio>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
  BMP_RGB = 0,
  BMP_RLE8 = 1,
  BMP_RLE4 = 2,
  BMP_BITFIELDS = 3,
  BMP_JPEG = 4,
  BMP_PNG = 5,
  BMP_ALPHABITFIELDS = 6,
  BMP_CMYK = 11,
  BMP_CMYKRLE8 = 12,
  BMP_CMYKRLE4 = 13
} BMP_COMPRESSION;

typedef struct {
  uint32_t size;
  uint32_t pdata;
  uint32_t header_size;
  int32_t width;
  int32_t height;
  int16_t planes;
  uint16_t bits;
  BMP_COMPRESSION compression;
  uint32_t imagesize;
  uint32_t colors;
  int32_t hres;
  int32_t vres;
  uint32_t importantcolors;
  uint32_t red_mask;
  uint32_t green_mask;
  uint32_t blue_mask;
} BMP;

void bmp_print_header(const BMP* bmp);
int bmp_write_header(BMP* bmp, FILE* fd);
int bmp_read_header(BMP* bmp, FILE* fd);
void bmp_write(BMP* bmp, const uint8_t *output, FILE *fp);
void bmp_read_pdata(const BMP* bmp, uint8_t *output, FILE *fp);
#ifdef __cplusplus
}
#endif