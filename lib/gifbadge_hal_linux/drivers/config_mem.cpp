/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include <cstring>
#include <csignal>
#include "drivers/config_mem.h"
void hal::config::oslinux::Config_Mem::setPath(const char *path) {
  strcpy(_path, path);
}
void hal::config::oslinux::Config_Mem::getPath(char *outPath) {
  strcpy(outPath, _path);
}
void hal::config::oslinux::Config_Mem::setLocked(bool) {

}
bool hal::config::oslinux::Config_Mem::getLocked() {
  return _locked;
}
bool hal::config::oslinux::Config_Mem::getSlideShow() {
  return _slideshow;
}
void hal::config::oslinux::Config_Mem::setSlideShow(bool slideshow) {
  _slideshow = slideshow;
}
int hal::config::oslinux::Config_Mem::getSlideShowTime() {
  return _slideshow_time;
}
void hal::config::oslinux::Config_Mem::setSlideShowTime(int time) {
  _slideshow_time = time;
}
int hal::config::oslinux::Config_Mem::getBacklight() {
  return _backlight;
}
void hal::config::oslinux::Config_Mem::setBacklight(int level) {
  _backlight = level;
}
void hal::config::oslinux::Config_Mem::reload() {

}
void hal::config::oslinux::Config_Mem::save() {

}
hal::config::oslinux::Config_Mem::Config_Mem() {
  getcwd(_path, sizeof(_path));
  strcat(_path, "/");
  strcat(_path, "data");
//  getcwd(_path, sizeof(_path));
}
