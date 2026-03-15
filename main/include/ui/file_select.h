/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#ifndef LVGL_FILE_SELECT_H
#define LVGL_FILE_SELECT_H
#include <lvgl.h>

#ifdef __cplusplus
extern "C"
{
#endif

void file_list(lv_obj_t *parent);
lv_obj_t *file_select(const char *top, const char *current, bool allow_folder);
void FileWindowClose(lv_event_t *e);

#ifdef __cplusplus
}
#endif
#endif //LVGL_FILE_SELECT_H
