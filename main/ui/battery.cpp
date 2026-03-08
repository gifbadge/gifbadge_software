/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include "hal/battery.h"
#include "ui/battery.h"

#include "hw_init.h"
#include "ui/style.h"
#include "ui/widgets/battery/lv_battery.h"


static void battery_percent_update(lv_obj_t *widget) {
  auto battery = static_cast<hal::battery::Battery *>(lv_obj_get_user_data(widget));
  if (battery->BatteryStatus() == hal::battery::Battery::State::ERROR || battery->BatteryStatus() == hal::battery::Battery::State::NOT_PRESENT) {
    lv_battery_set_value(widget, 0);
  } else {
    lv_battery_set_value(widget, battery->BatterySoc());
  }
}

static void battery_symbol_update(lv_obj_t *cont) {
  auto battery = static_cast<hal::battery::Battery *>(lv_obj_get_user_data(cont));
  lv_obj_t *symbol = lv_obj_get_child(cont, 0);
  lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_remove_flag(cont, LV_OBJ_FLAG_HIDDEN);
  if (battery->BatteryStatus() == hal::battery::Battery::State::ERROR) {
    lv_image_set_src(symbol, LV_SYMBOL_DUMMY "\uf22f");
    lv_obj_set_style_text_color(symbol, lv_color_hex(0xeed202), LV_PART_MAIN); //Yellow
  } else if (battery->BatteryStatus() == hal::battery::Battery::State::NOT_PRESENT) {
    lv_image_set_src(symbol, LV_SYMBOL_DUMMY "\ue5cd");
    lv_obj_set_style_text_color(symbol, lv_color_hex(0xff0033), LV_PART_MAIN); //Red
  } else if (battery->BatteryStatus() == hal::battery::Battery::State::CHARGING) {
    lv_image_set_src(symbol, LV_SYMBOL_DUMMY "\uea0b");
    lv_obj_set_style_text_color(symbol, lv_color_hex(0x50C878), LV_PART_MAIN); //Green
  } else {
    lv_image_set_src(symbol, LV_SYMBOL_DUMMY);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, LV_PART_MAIN);
  }
}



void battery_widget(lv_obj_t *scr) {
  lv_obj_t *battery_bar = lv_obj_create(scr);
  lv_obj_set_size(battery_bar, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_add_style(battery_bar, &style_battery_bar, LV_PART_MAIN);

  lv_obj_t *bar = lv_battery_create(battery_bar);
  lv_obj_add_style(bar, &style_battery_indicator, LV_PART_INDICATOR);
  lv_obj_add_style(bar, &style_battery_main, LV_PART_MAIN);
  lv_obj_set_size(bar, 20, 40);
  lv_obj_center(bar);
  lv_obj_set_user_data(bar, get_board()->GetBattery());


  lv_obj_add_event_cb(bar, [](lv_event_t *e) {
    battery_percent_update(static_cast<lv_obj_t *>(lv_event_get_target(e)));
  }, LV_EVENT_REFRESH, nullptr);
  battery_percent_update(bar);

  lv_timer_t *timer = lv_timer_create([](lv_timer_t *timer) {
    auto *obj = static_cast<lv_obj_t *>(timer->user_data);
    lv_obj_send_event(obj, LV_EVENT_REFRESH, nullptr);
  }, 30000, bar);
  lv_obj_add_event_cb(bar, [](lv_event_t *e) {
    auto *timer = static_cast<lv_timer_t *>(e->user_data);
    lv_timer_del(timer);
  }, LV_EVENT_DELETE, timer);

  lv_obj_t *battery_symbol_cont = lv_obj_create(battery_bar);
  lv_obj_remove_style_all(battery_symbol_cont);
  lv_obj_add_style(battery_symbol_cont, &style_battery_icon_container, LV_PART_MAIN);
  lv_obj_set_size(battery_symbol_cont, 24, 24);
  lv_obj_align_to(battery_symbol_cont, bar, LV_ALIGN_BOTTOM_RIGHT, 18, 12);

  lv_obj_t *battery_symbol = lv_img_create(battery_symbol_cont);
  lv_obj_add_style(battery_symbol, &style_battery_icon, LV_PART_MAIN);
  lv_obj_align(battery_symbol, LV_ALIGN_CENTER, 0, 0);

  lv_obj_set_user_data(battery_symbol_cont, get_board()->GetBattery());

  lv_obj_add_event_cb(battery_symbol_cont, [](lv_event_t *e) {
    battery_symbol_update(static_cast<lv_obj_t *>(lv_event_get_target(e)));
  }, LV_EVENT_REFRESH, nullptr);
  battery_symbol_update(battery_symbol_cont);

  lv_timer_t *timer_icon = lv_timer_create([](lv_timer_t *timer) {
    auto *obj = static_cast<lv_obj_t *>(timer->user_data);
    lv_obj_send_event(obj, LV_EVENT_REFRESH, nullptr);
  }, 30000, battery_symbol_cont);
  lv_obj_add_event_cb(battery_symbol_cont, [](lv_event_t *e) {
    auto *timer = static_cast<lv_timer_t *>(e->user_data);
    lv_timer_del(timer);
  }, LV_EVENT_DELETE, timer_icon);

}