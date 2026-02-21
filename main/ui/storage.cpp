/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include <cmath>
#include "ui/storage.h"

#include <dirent.h>

#include "ui/menu.h"
#include "ui/device_group.h"
#include "ui/widgets/file_list/file_list.h"
#include "ui/style.h"
#include "hw_init.h"
#ifdef ESP_PLATFORM

static const char *TAG = "storage.cpp";

struct StorageFields {
  lv_obj_t *container;
  lv_obj_t *space_bar;
  lv_obj_t *space_label;
};

static void StorageFormatClose(lv_event_t *e) {
  restore_group(static_cast<lv_obj_t *>(lv_event_get_user_data(e)));
}

static void StorageExit(lv_event_t *e) {
  auto *fields = static_cast<StorageFields *>(lv_event_get_user_data(e));
  lv_obj_delete(fields->container);
  free(fields);
}

static void StorageFormatCancel(lv_event_t *e) {
  lv_obj_delete(static_cast<lv_obj_t *>(lv_event_get_user_data(e)));
}

struct StorageFormatTimerData {
  lv_obj_t *storage_menu;
  lv_obj_t *format_wait;
  QueueHandle_t format_queue;
  TaskHandle_t format_task;
};

static void StorageCheckDone(lv_timer_t *timer) {
  auto *fields = static_cast<StorageFormatTimerData *>(lv_timer_get_user_data(timer));
  esp_err_t ret;
  if (xQueueReceive(fields->format_queue, &ret, 0)) {
    LOGI(TAG, "Format finished with %i", ret);
    lv_obj_delete(fields->format_wait);
    lv_obj_send_event(fields->storage_menu, LV_EVENT_REFRESH, nullptr);
    vQueueDelete(fields->format_queue);
    free(fields);
    if (ret != ESP_OK) {
      lv_obj_t *container = lv_obj_create(lv_scr_act());
      lv_obj_set_size(container, LV_PCT(100), LV_PCT(100));
      lv_obj_set_flex_flow(container, LV_FLEX_FLOW_ROW);
      lv_obj_set_flex_align(container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
      lv_obj_t *img = lv_img_create(container);
      lv_img_set_src(img, "\uf568");
      lv_obj_add_style(img, &icon_style, LV_PART_MAIN);
      lv_obj_t *label1 = lv_label_create(container);
      lv_obj_add_flag(label1, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
      lv_label_set_text_fmt(label1, "Format Failed due to:");
      lv_obj_t *err = lv_label_create(container);
      lv_obj_add_flag(err, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
      lv_label_set_text(err, esp_err_to_name(ret));
      lv_obj_t *label2 = lv_label_create(container);
      lv_obj_add_flag(label2, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
      lv_label_set_text(label2, "Reset the device, and try again");
    }
  }
}

static void StorageFormatTask(void *arg) {
  auto *fields = static_cast<StorageFormatTimerData *>(arg);
  esp_err_t ret;
  ret = get_board()->StorageFormat();
  xQueueSendToBack(fields->format_queue, &ret, 0);
  vTaskDelete(nullptr);
}

static void StorageFormatOK(lv_event_t *e) {
  auto *fields = static_cast<StorageFormatTimerData *>(malloc(sizeof(StorageFormatTimerData)));

  fields->storage_menu =
      static_cast<lv_obj_t *>(lv_obj_get_user_data(static_cast<lv_obj_t *>(lv_event_get_user_data(e))));

  lv_obj_delete(static_cast<lv_obj_t *>(lv_event_get_user_data(e)));
  lv_obj_t *container = lv_obj_create(lv_scr_act());
  fields->format_wait = container;
  lv_obj_set_size(container, LV_PCT(100), LV_PCT(100));
  lv_obj_set_flex_flow(container, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_t *spinner = lv_spinner_create(container);
  lv_spinner_set_anim_params(spinner, 3000, 200);
  lv_obj_t *label = lv_label_create(container);
  lv_obj_add_flag(label, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
  lv_label_set_text(label, "Formatting");

  lv_timer_t *timer = lv_timer_create(StorageCheckDone, 5000, fields);
  lv_obj_add_event_cb(container, [](lv_event_t *e) {
    auto *timer = (lv_timer_t *) e->user_data;
    lv_timer_del(timer);
  }, LV_EVENT_DELETE, timer);

  fields->format_queue = xQueueCreate(1, sizeof(int));
  xTaskCreate(StorageFormatTask, "FormatTask", 10000, fields, 2, nullptr);

}

static void StorageRefresh(lv_event_t *e) {
  auto *fields = static_cast<StorageFields *>(lv_event_get_user_data(e));
  StorageInfo info = get_board()->GetStorageInfo();
  lv_bar_set_value(fields->space_bar, static_cast<int>((info.free_bytes +  info.total_bytes/2)/info.total_bytes), LV_ANIM_OFF);
  lv_label_set_text_fmt(fields->space_label, "%llu/%lluMB", (info.total_bytes - info.free_bytes)/1000000, info.total_bytes/1000000);
}

static void StorageFormat(lv_event_t *e) {
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
  lv_label_set_text(warning_text, "This will erase all data on device");
//
  lv_obj_t *ok_btn = lv_btn_create(container);
  lv_obj_add_flag(ok_btn, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
  lv_obj_t *ok_text = lv_label_create(ok_btn);
  lv_label_set_text(ok_text, "OK");
  lv_obj_t *cancel_btn = lv_btn_create(container);
  lv_obj_t *cancel_text = lv_label_create(cancel_btn);
  lv_label_set_text(cancel_text, "Cancel");

  lv_obj_add_event_cb(ok_btn, StorageFormatOK, LV_EVENT_CLICKED, container);
  lv_obj_add_event_cb(cancel_btn, StorageFormatCancel, LV_EVENT_CLICKED, container);
  lv_obj_add_event_cb(container, StorageFormatClose, LV_EVENT_DELETE, lv_event_get_target(e));
  lv_obj_set_user_data(container, lv_event_get_target(e));

  lv_group_focus_obj(cancel_btn);

}

static void clear_cache(lv_event_t *e) {
  char cache_path[128];
  dirent *de;
  strcpy(cache_path, get_board()->GetStoragePath());
  strcat(cache_path, "/.cache/");
  DIR *dir = opendir(cache_path);
  while (de = readdir(dir), de != nullptr) {
    char path[128];
    strcpy(path, cache_path);
    strcat(path, de->d_name);
    remove(path);
  }
}

lv_obj_t *storage_menu() {
  new_group();
  lv_obj_t *cont_flex = lv_file_list_create(lv_scr_act());
  lv_file_list_icon_style(cont_flex, &icon_style);

  StorageInfo info = get_board()->GetStorageInfo();

  auto *fields = static_cast<StorageFields *>(malloc(sizeof(StorageFields)));
  assert(fields != nullptr);
  fields->container = cont_flex;

  lv_obj_t *size_btn;
  switch (info.type) {
    case StorageType_None:
      size_btn = lv_file_list_add(cont_flex, nullptr);
      break;
    case StorageType_SPI:
      size_btn = lv_file_list_add(cont_flex, "\uf80e");
      break;
    case StorageType_SDIO:
    case StorageType_MMC:
    case StorageType_SD:
    case StorageType_SDHC:
      size_btn = lv_file_list_add(cont_flex, "\ue623");
      break;
    default:
      size_btn = lv_file_list_add(cont_flex, nullptr);
      break;
  }
  lv_obj_t *space_bar = lv_bar_create(size_btn);
  fields->space_bar = space_bar;
  lv_obj_set_style_bg_color(space_bar, lv_color_white(), LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(space_bar, lv_color_black(), LV_PART_MAIN);
  lv_obj_set_flex_grow(space_bar, 1);
  lv_bar_set_range(space_bar, 0, 100);
  lv_obj_t *size_label = lv_label_create(space_bar);
  fields->space_label = size_label;
  lv_obj_center(size_label);

  lv_obj_t *format_btn = lv_file_list_add(cont_flex, nullptr);
  lv_obj_add_style(format_btn, &menu_font_style, LV_PART_MAIN);
  lv_obj_t *format_label = lv_label_create(format_btn);
  lv_label_set_text(format_label, "Format Storage");

  lv_obj_t *clear_cache_btn = lv_file_list_add(cont_flex, nullptr);
  lv_obj_add_style(clear_cache_btn, &menu_font_style, LV_PART_MAIN);
  lv_obj_t *clear_cache_label = lv_label_create(clear_cache_btn);
  lv_label_set_text(clear_cache_label, "Clear Image Cache");

  lv_obj_t *exit_button = lv_file_list_add(cont_flex, "\ue5c9");
  lv_obj_t *exit = lv_obj_create(exit_button);
  lv_obj_add_flag(exit, LV_OBJ_FLAG_HIDDEN);
  lv_group_remove_obj(exit);

  lv_obj_add_event_cb(format_btn, StorageFormat, LV_EVENT_CLICKED, cont_flex);
  lv_obj_add_event_cb(clear_cache_btn, clear_cache, LV_EVENT_CLICKED, cont_flex);
  lv_obj_add_event_cb(exit, StorageExit, LV_EVENT_CLICKED, fields);
  lv_obj_add_event_cb(cont_flex, StorageRefresh, LV_EVENT_REFRESH, fields);
  lv_obj_send_event(cont_flex, LV_EVENT_REFRESH, nullptr);

  lv_file_list_scroll_to_view(cont_flex, 0);

  return cont_flex;
}

#else
lv_obj_t *storage_menu() {
  return nullptr;
}
#endif