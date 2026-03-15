/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once
#include <memory>
#include <nvs_handle.hpp>

#include "hal/board.h"
#include "hal/config_storage.h"

namespace hal::config::esp32s3 {
class Config_NVS: public Config {
 public:
  void format() override;

  Config_NVS(Boards::Board *board);
  ~Config_NVS() override = default;

  void setPath(const char *) override;
  void getPath(char *outPath) override;
  void setLocked(bool) override;
  bool getLocked() override;
  bool getSlideShow() override;
  void setSlideShow(bool) override;
  int getSlideShowTime() override;
  void setSlideShowTime(int) override;
  int getBacklight() override;
  void setBacklight(int) override;
  void reload() override;
  void save() override;
  void getCard(cards, char *path) override;
  void setCard(cards, const char *path) override;


 private:
  std::unique_ptr<nvs::NVSHandle> handle;
  template<typename T>
  T get_item_or_default(const char *key, T value);
  void get_string_or_default(const char *item, const char* default_value, char *out, size_t out_len);
  Boards::Board *_board;
};
}

