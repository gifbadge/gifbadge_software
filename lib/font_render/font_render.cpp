/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include "include/font_render.h"
#include "fonts.h"

#include <mcufont.h>
#include <cstring>
#include <sys/types.h>

#ifndef __bswap16
#define __bswap16 __bswap_16
#endif

/********************************************
 * Callbacks to specify rendering behaviour *
 ********************************************/

typedef struct {
    uint8_t *buffer;
    int16_t width;
    int16_t height;
    uint16_t y;
    bool justify;
    int16_t margin;
    int16_t anchor;
    enum mf_align_t alignment;
    const struct mf_font_s *font;
} state_t;

uint16_t alphaBlendRGB565( uint8_t alpha, uint32_t fg, uint32_t bg){
    alpha = ( alpha + 4 ) >> 3;
    bg = (bg | (bg << 16)) & 0b00000111111000001111100000011111;
    fg = (fg | (fg << 16)) & 0b00000111111000001111100000011111;
    uint32_t result = ((((fg - bg) * alpha) >> 5) + bg) & 0b00000111111000001111100000011111;
    return (uint16_t)((result >> 16) | result);
}

/* Callback to write to a memory buffer. */
static void pixel_callback(int16_t x, int16_t y, uint8_t count, uint8_t alpha,
                           void *state)
{
    auto *s = (state_t*)state;
    uint32_t pos;

    if (y < 0 || y >= s->height) return;
    if (x < 0 || x + count >= s->width) return;

    auto *tmp_buffer = (uint16_t *)s->buffer;
    while (count--)
    {
        pos = (uint32_t)s->width * y + x;
        tmp_buffer[pos] = alphaBlendRGB565(alpha, 0xFFFF, 0x2966);

        x++;
    }
}

/* Callback to render characters. */
static uint8_t character_callback(int16_t x, int16_t y, mf_char character,
                                  void *state)
{
    auto *s = (state_t*)state;
    return mf_render_character(s->font, x, y, character, pixel_callback, state);
}

/* Callback to render lines. */
static bool line_callback(const char *line, uint16_t count, void *state)
{
    auto *s = (state_t*)state;

    if (s->justify)
    {
        mf_render_justified(s->font, s->anchor, s->y,
                            s->width - s->margin * 2,
                            line, count, character_callback, state);
    }
    else
    {
        mf_render_aligned(s->font, s->anchor, s->y,
                          s->alignment, line, count,
                          character_callback, state);
    }
    s->y += s->font->line_height;
    return true;
}

/* Callback to just count the lines.
 * Used to decide the image height */
bool count_lines(const char *, uint16_t, void *state)
{
    int *linecount = (int*)state;
    (*linecount)++;
    return true;
}

int render_text_centered(int16_t x_max, int16_t y_max, int16_t margin, const char *text, uint8_t *buffer)
{
    int height;
    const struct mf_font_s *font = (struct mf_font_s*)&mf_rlefont_DejaVuSerif16;
    state_t state = {};


    /* Count the number of lines that we need. */
    height = 0;
    mf_wordwrap(font, x_max - 2 * margin,
                text, count_lines, &height);
    height *= font->height;
    height += 4;

    /* Allocate and clear the image buffer */
    state.width = x_max;
    state.height = y_max;
    state.buffer = buffer;
    state.y = (y_max/2)-((height+1)/2);

    state.font = font;
    state.alignment = MF_ALIGN_CENTER;
    state.anchor = x_max/2;

    /* Render the text */
    mf_wordwrap(font, x_max - 2 * margin,
                text, line_callback, &state);

    return 0;
}