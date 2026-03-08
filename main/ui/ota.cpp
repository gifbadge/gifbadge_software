/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/


#include <lvgl.h>

#include "ui/ota.h"

#include "hw_init.h"
#include "ui/menu.h"
#include "ui/style.h"

static void ota_percent_update(lv_obj_t *widget) {
  int status = get_board()->OtaStatus();
  if (status > 0) {
    lv_bar_set_value(widget, status, LV_ANIM_OFF);
  }
}

void lvgl_ota() {
  // destroy_screens();
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
  lv_image_set_src(label, ICON_UPDATE);
  lv_obj_t * bar = lv_bar_create(cont);
  lv_bar_set_value(bar, 0, LV_ANIM_OFF);

  lv_obj_add_event_cb(bar,
                      [](lv_event_t *e) {
                        ota_percent_update(static_cast<lv_obj_t *>(lv_event_get_target(e)));
                      },
                      LV_EVENT_REFRESH,
                      nullptr);
  ota_percent_update(bar);

  lv_timer_t *timer = lv_timer_create([](lv_timer_t *timer) {
                                        auto *obj = static_cast<lv_obj_t *>(timer->user_data);
                                        lv_obj_send_event(obj, LV_EVENT_REFRESH, nullptr);
                                      },
                                      500,
                                      bar);
  lv_obj_add_event_cb(bar,
                      [](lv_event_t *e) {
                        auto *timer = static_cast<lv_timer_t *>(e->user_data);
                        lv_timer_delete(timer);
                      },
                      LV_EVENT_DELETE,
                      timer);
}