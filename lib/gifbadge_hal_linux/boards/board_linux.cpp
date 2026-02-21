/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include <cassert>
#include "boards/board_linux.h"

#include <unistd.h>

#include "hal/backlight.h"
#include "hal/battery.h"
#include "hal/display.h"
#include "hal/keys.h"
#include "hal/touch.h"
#include "drivers/backlight_dummy.h"
#include "drivers/display_sdl.h"
#include "drivers/touch_sdl.h"

board_linux::board_linux() {
  _backlight = new hal::backlight::oslinux::backlight_dummy();
  assert(_backlight != nullptr);
  _battery = new hal::battery::oslinux::battery_dummy();
  assert(_battery != nullptr);
  _display = displaySdl;
  assert(_display != nullptr);
  _config = new hal::config::oslinux::Config_Mem();
  assert(_config != nullptr);
  _keys = keysSdl;
  assert(_keys != nullptr);
  _touch = touchSdl;
  assert(_touch != nullptr);

  getcwd(_storagePath, 128);



}
hal::battery::Battery *board_linux::GetBattery() {
  return _battery;
}
hal::touch::Touch *board_linux::GetTouch() {
  return _touch;
}
hal::keys::Keys *board_linux::GetKeys() {
  return _keys;
}
hal::display::Display *board_linux::GetDisplay() {
  return _display;
}
hal::backlight::Backlight *board_linux::GetBacklight() {
  return _backlight;
}
hal::vbus::Vbus * board_linux::GetVbus() {
  return Board::GetVbus();
}
void board_linux::PowerOff() {

}
Boards::BoardPower board_linux::PowerState() {
  return Boards::BOARD_POWER_NORMAL;
}
bool board_linux::StorageReady() {
  return true;
}
StorageInfo board_linux::GetStorageInfo() {
  return StorageInfo();
}

const char * board_linux::GetStoragePath() {
  return _storagePath;
}

const char *board_linux::Name() {
  return "Linux";
}
hal::config::Config *board_linux::GetConfig() {
  return _config;
}
void board_linux::DebugInfo() {
  _display->update();
}
bool board_linux::UsbConnected() {
  return false;
}

hal::charger::Charger * board_linux::GetCharger() {
  return Board::GetCharger();
}
void board_linux::BootInfo() {
  Board::BootInfo();
}
bool board_linux::OtaCheck() {
  return Board::OtaCheck();
}
Boards::OtaError board_linux::OtaValidate() {
  return Board::OtaValidate();
}
void board_linux::OtaInstall() {
  Board::OtaInstall();
}
int board_linux::OtaStatus() {
  return Board::OtaStatus();
}
Boards::WakeupSource board_linux::BootReason() {
  return Board::BootReason();
}
void board_linux::LateInit() {
  _buffer = malloc(_display->size.first*_display->size.second + 0x6100);

}
void board_linux::Reset() {
}
const char * board_linux::SwVersion() {
  return "Blah";
}
char * board_linux::SerialNumber() {
  return "None";
}
