/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once

#include <string>
#include <mutex>

namespace hal::config {

enum cards {
  UP,
  DOWN
};

class Config {
 public:
  Config() = default;
  virtual ~Config() = default;
  virtual void save() = 0;

  virtual void setPath(const char *) = 0;
  virtual void getPath(char *outPath) = 0;
  virtual void setLocked(bool) = 0;
  virtual bool getLocked() = 0;
  virtual bool getSlideShow() = 0;
  virtual void setSlideShow(bool) = 0;
  virtual int getSlideShowTime() = 0;
  virtual void setSlideShowTime(int) = 0;
  virtual int getBacklight() = 0;
  virtual void setBacklight(int) = 0;
  virtual void reload() = 0;
  virtual void getCard(cards, char *path) = 0;
  virtual void setCard(cards, const char *path) = 0;
  virtual void format() {}

};
}

