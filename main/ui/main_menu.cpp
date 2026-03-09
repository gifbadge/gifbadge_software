/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include <lvgl.h>
#include <cassert>

#include "ui/main_menu.h"
#include "ui/file_options.h"
#include "ui/widgets/file_list/file_list.h"
#include "ui/menu.h"
#include "display.h"
#include "ui/style.h"
#include "ui/device_group.h"
#include "ui/storage.h"
#include "hw_init.h"
#include "ui/device_info.h"
#include "ui/help.h"
#include "ui/no_sd.h"

lv_obj_t *main_menu_obj = nullptr;

static lv_obj_t * exit_callback() {
  if (!get_board()->StorageReady()) {
    lv_obj_del(main_menu_obj);
    lvgl_nosd();
    return nullptr;
  }
  TaskHandle_t handle = xTaskGetHandle("LVGL");
  xTaskNotifyIndexed(handle, 0, LVGL_STOP, eSetValueWithOverwrite);
  return nullptr;
}

static void BacklightSliderExitCallback(lv_event_t *e) {
  lv_obj_t *obj = lv_event_get_target_obj(e);
  get_board()->GetConfig()->setBacklight(lv_slider_get_value(obj));
  restore_group(static_cast<lv_obj_t *>(lv_event_get_user_data(e)));
}

static lv_obj_t *ShutdownCallback() {
  get_board()->PowerOff();
  return nullptr;
}

static lv_obj_t * help_callback() {
  // TaskHandle_t handle = xTaskGetHandle("LVGL");
  // xTaskNotifyIndexed(handle, 0, LVGL_STOP, eSetValueWithOverwrite);
  return nullptr;
}

static void BacklightSliderChangedCallback(lv_event_t *e) {
  lv_obj_t *obj = lv_event_get_target_obj(e);
  int level = lv_slider_get_value(obj) * 10;
  get_board()->GetBacklight()->setLevel(level);
}

static void mainMenuCleanup(lv_event_t *e){
  lv_group_delete(static_cast<lv_group_t *>(lv_event_get_user_data(e)));
}

typedef struct {
  MenuType callback;
} subMenuData;

static void cleanupSubMenu(lv_event_t *e){
  free(lv_event_get_user_data(e));
}

static void closeSubMenu(lv_event_t *e) {
  auto *root_obj = static_cast<lv_obj_t *>(lv_event_get_user_data(e));
  if(lv_obj_is_valid(root_obj)){
    lv_obj_t *obj = lv_obj_get_parent(root_obj);
    restore_group(obj);
    lv_obj_t *scr = lv_obj_get_screen(obj);
    if(lv_obj_is_valid(scr)) {
      lv_screen_load(lv_obj_get_screen(scr));
    }
  }
}

static void openSubMenu(lv_event_t *e){
  auto *obj = static_cast<lv_obj_t *>(lv_event_get_target(e));
  auto *data = static_cast<subMenuData *>(lv_event_get_user_data(e));
  LV_LOG_USER("%s", lv_label_get_text(obj));
  lv_screen_load(create_screen());
  assert(data->callback != nullptr);
  lv_obj_t *window = data->callback();
  if(window) {
    lv_obj_add_event_cb(window, closeSubMenu, LV_EVENT_DELETE, obj);
  }
}

static lv_obj_t *subMenu(lv_obj_t *parent, const char *name, MenuType menu){
  assert(menu != nullptr);
  lv_obj_t *button = lv_file_list_add(parent, nullptr);
  lv_obj_t *label = lv_label_create(button);
  lv_obj_add_style(label, &menu_font_style, LV_PART_MAIN);
  lv_label_set_text(label, name);

  auto *data = static_cast<subMenuData *>(malloc(sizeof(subMenuData)));
  assert(data != nullptr);
  data->callback = menu;

  lv_obj_add_event_cb(label, openSubMenu, LV_EVENT_CLICKED, data);
  lv_obj_add_event_cb(label, cleanupSubMenu, LV_EVENT_DELETE, data);
  return button;
}

void main_menu() {
    lv_obj_t *scr = lv_scr_act();
    new_group();
    main_menu_obj = lv_file_list_create(scr);
    lv_file_list_icon_style(main_menu_obj, &icon_style);

    lv_obj_t *backlight_button = lv_file_list_add(main_menu_obj, "\ue3ab");
    lv_obj_t *slider = lv_slider_create(backlight_button);
    lv_obj_set_width(slider, lv_pct(80));
    lv_obj_set_style_bg_color(slider, lv_color_white(), LV_PART_KNOB);
    lv_obj_set_style_bg_color(slider, lv_color_white(), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, lv_color_black(), LV_PART_MAIN);
    lv_slider_set_range(slider, 1, 10);
    lv_group_t *slider_group = lv_group_create();
    lv_group_add_obj(slider_group, slider);
    lv_obj_add_event_cb(slider, BacklightSliderChangedCallback, LV_EVENT_VALUE_CHANGED, backlight_button);
    lv_obj_add_event_cb(slider, BacklightSliderExitCallback, LV_EVENT_RELEASED, backlight_button);
    lv_slider_set_value(slider, get_board()->GetConfig()->getBacklight(), LV_ANIM_OFF);

    if (last_fps > 0) {
      lv_obj_t *fps = lv_file_list_add(main_menu_obj, nullptr);
      lv_obj_add_style(fps, &menu_font_style, LV_PART_MAIN);
      lv_obj_t *fps_text = lv_label_create(fps);
      char fps_text_str[64];
      snprintf(fps_text_str, 63, "GIF FPS: %.0f", last_fps);
      lv_label_set_text(fps_text, fps_text_str);
    }

    lv_obj_t *file_menu = subMenu(main_menu_obj, "File Select", &FileOptions);
    lv_obj_t *storagemenu = subMenu(main_menu_obj, "Storage", &storage_menu);
    if (!get_board()->StorageReady()) {
      lv_obj_add_state(file_menu, LV_STATE_DISABLED);
      lv_obj_add_state(storagemenu, LV_STATE_DISABLED);
    }
    subMenu(main_menu_obj, "Device Info", &device_info);
    subMenu(main_menu_obj, "Shutdown", &ShutdownCallback);
    subMenu(main_menu_obj, "Help", &help);
    subMenu(main_menu_obj, "Exit", &exit_callback);

    lv_obj_add_event_cb(scr, mainMenuCleanup, LV_EVENT_DELETE, lv_group_get_default());

    lv_file_list_scroll_to_view(main_menu_obj, 0);
}

