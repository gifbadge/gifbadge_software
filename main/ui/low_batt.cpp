/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include "ui/low_batt.h"
#include "lvgl.h"
#include "ui/style.h"

void lvgl_low_batt() {
  lv_obj_t *scr = lv_screen_active();
  lv_screen_load(scr);
  lv_obj_t *cont = lv_obj_create(scr);
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_width(cont, LV_PCT(100));
  lv_obj_set_height(cont, LV_PCT(100));
  lv_obj_set_flex_grow(cont, 1);
  lv_obj_set_scroll_dir(cont, LV_DIR_NONE);
  lv_obj_t *label = lv_image_create(cont);
  lv_obj_set_style_text_font(label, &material_icons_special, LV_PART_MAIN);
  lv_image_set_src(label, ICON_LOW_BATTERY);
  lv_obj_t *error_label = lv_label_create(cont);
  lv_label_set_text(error_label, "Low Battery");
  lv_obj_t *error_label2 = lv_label_create(cont);
  lv_label_set_text(error_label2, "Device will Power Off");
}
