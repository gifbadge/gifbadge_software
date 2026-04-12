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
    lv_obj_t *container = lv_obj_create(lv_scr_act());
    new_group();
    lv_obj_set_size(container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *warning = lv_label_create(container);
    lv_label_set_text(warning, "WARNING");
    lv_obj_set_style_text_color(warning, lv_color_hex(0xFF0000), LV_PART_MAIN);
    lv_obj_set_style_text_decor(warning, LV_TEXT_DECOR_UNDERLINE, LV_PART_MAIN);
    lv_obj_t *warning_text = lv_label_create(container);
    lv_obj_add_flag(warning_text, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
    lv_label_set_text(warning_text, "This will reset all settings");
    //
    lv_obj_t *ok_btn = lv_btn_create(container);
    lv_obj_add_flag(ok_btn, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
    lv_obj_t *ok_text = lv_label_create(ok_btn);
    lv_label_set_text(ok_text, "OK");
    lv_obj_t *cancel_btn = lv_btn_create(container);
    lv_obj_t *cancel_text = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_text, "Cancel");

    lv_obj_add_event_cb(ok_btn, reset_callback, LV_EVENT_CLICKED, container);
    lv_obj_add_event_cb(cancel_btn, exit_callback, LV_EVENT_CLICKED, container);
    lv_group_focus_obj(cancel_btn);


  return container;
}
