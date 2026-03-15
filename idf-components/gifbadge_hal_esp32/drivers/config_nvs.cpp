/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include <nvs_flash.h>
#include "log.h"
#include "drivers/config_nvs.h"

hal::config::esp32s3::Config_NVS::Config_NVS(Boards::Board *board): _board(board) {
  esp_err_t err;
  err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    // NVS partition was truncated and needs to be erased
    // Retry nvs_flash_init
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);
  handle = nvs::open_nvs_handle("image", NVS_READWRITE, &err);
}

template<typename T>
T hal::config::esp32s3::Config_NVS::get_item_or_default(const char *item, T value) {
  esp_err_t err;
  T ret;
  err = handle->get_item(item, ret);
  switch (err) {
    case ESP_OK:
      return ret;
    case ESP_ERR_NVS_NOT_FOUND:
      handle->set_item(item, value);
      handle->commit();
      return value;
    default :
      return value;
  }
}

void hal::config::esp32s3::Config_NVS::get_string_or_default(const char *item, const char *default_value, char *out, size_t out_len) {
  if (handle->get_string(item, out, out_len) == ESP_ERR_NVS_NOT_FOUND) {
    handle->set_string(item, default_value);
    handle->commit();
    strncpy(out, default_value, out_len);
  }
}

void hal::config::esp32s3::Config_NVS::setPath(const char *path) {
  handle->set_string("path", path);
}

void hal::config::esp32s3::Config_NVS::getPath(char *outPath) {
  get_string_or_default("path", (const char *) "/data", outPath, 128);
}

void hal::config::esp32s3::Config_NVS::setLocked(bool locked) {
  handle->set_item("locked", locked);
}

bool hal::config::esp32s3::Config_NVS::getLocked() {
  return get_item_or_default("locked", false);
}

bool hal::config::esp32s3::Config_NVS::getSlideShow() {
  return get_item_or_default("slideshow", false);
}

void hal::config::esp32s3::Config_NVS::setSlideShow(bool slideshow) {
  handle->set_item("slideshow", slideshow);
}

int hal::config::esp32s3::Config_NVS::getSlideShowTime() {
  return get_item_or_default("slideshow_time", 15);
}

void hal::config::esp32s3::Config_NVS::setSlideShowTime(int slideshow_time) {
  handle->set_item("slideshow_time", slideshow_time);
}

int hal::config::esp32s3::Config_NVS::getBacklight(){
  return get_item_or_default("backlight", 10);
}

void hal::config::esp32s3::Config_NVS::setBacklight(int backlight) {
  handle->set_item("backlight", backlight);
}


void hal::config::esp32s3::Config_NVS::reload() {

}
void hal::config::esp32s3::Config_NVS::save() {

}
void hal::config::esp32s3::Config_NVS::getCard(cards card, char *path) {
  char card_str[8];
  snprintf(card_str, sizeof(card_str), "card_%0d", static_cast<int>(card));
  get_string_or_default(card_str, "/data", path, 128);
}
void hal::config::esp32s3::Config_NVS::setCard(cards card, const char *path) {
  char card_str[8];
  snprintf(card_str, sizeof(card_str), "card_%0d", static_cast<int>(card));
  handle->set_string(card_str, path);
}

void hal::config::esp32s3::Config_NVS::format() {
  nvs_flash_erase();
  _board->Reset();
}
