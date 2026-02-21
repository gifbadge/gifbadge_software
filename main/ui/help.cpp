/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include "ui/help.h"

#include <cstring>

#include "ui/menu.h"
#include "ui/device_group.h"
#include "ui/widgets/file_list/file_list.h"
#include "ui/style.h"

static void exit_callback(lv_event_t *e) {
  lv_obj_del(static_cast<lv_obj_t *>(lv_event_get_user_data(e)));
}

lv_obj_t *help() {
    new_group();
    lv_obj_t *cont_flex = lv_obj_create(lv_screen_active());

    lv_obj_set_flex_flow(cont_flex, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont_flex, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_width(cont_flex, LV_PCT(100));
    lv_obj_set_height(cont_flex, LV_PCT(100));
    lv_obj_set_flex_grow(cont_flex, 1);
    lv_obj_set_scroll_dir(cont_flex, LV_DIR_NONE);

    /*main container style*/
    lv_obj_set_style_radius(cont_flex, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_pad_bottom(cont_flex, 50, LV_PART_MAIN);
    lv_obj_set_style_pad_top(cont_flex, 50, LV_PART_MAIN);
    lv_obj_set_style_border_side(cont_flex, LV_BORDER_SIDE_NONE, LV_PART_MAIN);

    // lv_obj_t * dummy = lv_obj_create(cont_flex);
    // lv_obj_set_width(dummy, LV_PCT(100));
    // lv_obj_set_height(dummy, LV_PCT(30));


    lv_color_t bg_color = lv_color_black();
    lv_color_t fg_color = lv_color_white();

    // lv_obj_t * qr_cont = lv_obj_create(cont_flex);

    // lv_obj_set_flex_flow(qr_cont, LV_FLEX_FLOW_ROW);
    // lv_obj_set_width(qr_cont, LV_PCT(100));
    // lv_obj_set_height(qr_cont, LV_SIZE_CONTENT);
    lv_obj_t * qr = lv_qrcode_create(cont_flex);
    lv_obj_set_height(qr, LV_SIZE_CONTENT);
    lv_obj_set_width(qr, LV_PCT(100));
    // lv_obj_set_flex_flow(qr, LV_FLEX_FLOW_ROW);
    // lv_obj_set_flex_align(qr, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_qrcode_set_size(qr, 200);
    lv_qrcode_set_dark_color(qr, bg_color);
    lv_qrcode_set_light_color(qr, fg_color);
    // lv_obj_center(qr);

    /*Set data*/
    const char * data = "https://gifbadge.ca/manual/manual.html";
    lv_qrcode_update(qr, data, strlen(data));
    lv_obj_center(qr);

    lv_obj_t *text = lv_label_create(cont_flex);
    lv_label_set_text(text, "https://gifbadge.ca/manual");
    lv_obj_add_style(text, &menu_font_style, LV_PART_MAIN);


    lv_obj_t *exit_btn = lv_button_create(cont_flex);
    lv_obj_set_width(exit_btn, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(exit_btn, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(exit_btn, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *exit_label = lv_label_create(exit_btn);
    lv_obj_add_style(exit_label, &menu_font_style, LV_PART_MAIN);
    lv_label_set_text(exit_label, "Exit");
    lv_obj_add_event_cb(exit_btn, exit_callback, LV_EVENT_CLICKED, cont_flex);

    return cont_flex;
}