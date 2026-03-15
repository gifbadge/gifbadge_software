/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include "ui/reset_settings.h"


#include "ui/menu.h"
#include "ui/device_group.h"
#include "ui/widgets/file_list/file_list.h"
#include "ui/style.h"
#include "hw_init.h"

static void exit_callback(lv_event_t *e) {
  lv_obj_del(static_cast<lv_obj_t *>(lv_event_get_user_data(e)));
}

static void reset_callback(lv_event_t *e) {
  get_board()->GetConfig()->format();
  lv_obj_del(static_cast<lv_obj_t *>(lv_event_get_user_data(e)));
}



lv_obj_t *reset_settings() {
  new_group();
  lv_obj_t *cont_flex = lv_file_list_create(lv_scr_act());
  lv_file_list_icon_style(cont_flex, &icon_style);

  lv_obj_t *reset_header_text = lv_file_list_add(cont_flex, nullptr);
  lv_obj_add_style(reset_header_text, &menu_font_style, LV_PART_MAIN);
  lv_obj_t *reset_text = lv_label_create(reset_header_text);
  lv_label_set_text(reset_text, "This will reset all settings to defaults");

  lv_obj_t *reset_btn = lv_file_list_add(cont_flex, nullptr);
  lv_obj_t *reset_label = lv_label_create(reset_btn);
  lv_obj_add_style(reset_label, &menu_font_style, LV_PART_MAIN);
  lv_label_set_text(reset_label, "Reset");

  lv_obj_t *exit_btn = lv_file_list_add(cont_flex, nullptr);
  lv_obj_t *exit_label = lv_label_create(exit_btn);
  lv_obj_add_style(exit_label, &menu_font_style, LV_PART_MAIN);
  lv_label_set_text(exit_label, "Cancel");

  lv_obj_add_event_cb(reset_label, reset_callback, LV_EVENT_CLICKED, cont_flex);
  lv_obj_add_event_cb(exit_label, exit_callback, LV_EVENT_CLICKED, cont_flex);

  lv_file_list_scroll_to_view(cont_flex, 0);

  return cont_flex;
}
