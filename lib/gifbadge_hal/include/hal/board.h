/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once

#include <memory>
#include "hal/battery.h"
#include "hal/touch.h"
#include "keys.h"
#include "display.h"
#include "hal/backlight.h"
#include "hal/storage.h"
//#include "ota.h"
#include "config_storage.h"
#ifdef ESP_PLATFORM
#include "tinyusb_msc.h"
#endif
#include "vbus.h"
#include "charger.h"
#include "battery.h"
#include "touch.h"

namespace Boards {

enum BoardPower {
  BOARD_POWER_NORMAL,
  BOARD_POWER_LOW,
  BOARD_POWER_CRITICAL,
};

/*!
 * OTA Validation return value
 */
enum class OtaError {
  OK = 0,
  WRONG_CHIP,
  WRONG_BOARD,
  SAME_VERSION,
  VERIFICATION_FAILED,
  FAIL,
};

enum class WakeupSource {
  NONE,
  VBUS,
  KEY,
  REBOOT,
};


/**
 * The Board class represents the hardware gifbadge is running on.
 */
class Board {
 public:
  /**
   * The constructor should initialize the bare minimum of hardware.
   * For example the power management IC to enable charging, and find the reason it was powered on
   */
  Board() = default;
  virtual ~Board() = default;

  /**
   * Initialize the rest of the needed hardware
   */
  virtual void LateInit() = 0;

  /**
   * Get the instance of Battery used by the current board
   * @return Battery * or nullptr
   * @see Battery
   */
  virtual hal::battery::Battery * GetBattery() = 0;

  /**
   * Get the instance of Touch used by the current board
   * @return Touch * or nullptr
   * @see Touch
   */
  virtual hal::touch::Touch * GetTouch() = 0;

  /**
  * Get the instance of Keys used by the current board
  * @return Keys * or nullptr
   * @see Touch
  */
  virtual hal::keys::Keys * GetKeys() = 0;

  /**
  * Get the instance of Display used by the current board
  * @return Display * or nullptr
   * @see Keys
  */
  virtual hal::display::Display * GetDisplay() = 0;

  /**
  * Get the instance of Backlight used by the current board
  * @return Backlight * or nullptr
   * @see Display
  */
  virtual hal::backlight::Backlight * GetBacklight() = 0;

  /**
  * Get the instance of Vbus used by the current board
  * @return Vbus * or nullptr
   * @see Vbus
  */
  virtual hal::vbus::Vbus *GetVbus() { return nullptr;};

  /**
  * Get the instance of Charger used by the current board
  * @return Charger * or nullptr
   * @see Charger
  */
  virtual hal::charger::Charger *GetCharger() {return nullptr;};

  /**
  * Get the instance of Config used by the current board
  * @return Config * or nullptr
   * @see Config
  */
  virtual hal::config::Config *GetConfig() = 0;

  /**
   * power off if the hardware supports it. If not, deep sleep instead
   */
  virtual void PowerOff() = 0;

  /**
   * reset the device
   */
  virtual void Reset() = 0;

  /**
   * Get the power state to decide if a warning should be displayed or a power off should occur
   * @return
   */
  virtual BoardPower PowerState() = 0;

  /**
   * Acquire a power management lock to keep the device at maximum performance
   */
  virtual void PmLock() = 0;

  /**
   * Release the lock acquired by PmLock()
   */
  virtual void PmRelease() = 0;

  /**
   * Check if storage is present
   * @return true if storage is present otherwise false
   */
  virtual bool StorageReady() = 0;

  /**
   * returns a StorageInfo structure containing information about the storage present in the device
   * @return
   */
  virtual StorageInfo GetStorageInfo() = 0;

  /**
   * Formats the storage
   * @return 0 if successful, platform specific error otherwise
   */
  virtual int StorageFormat() = 0;

  /**
   *
   *
   */
  virtual const char *GetStoragePath() = 0;

  /**
   * Gets the pointer to a region of memory allocated for GIFDecoding
   * This should be allocated in internal/fast memory
   * Size should be (Screen x Resolution * Screen y Resolution)+0x6100
   * @return pointer to the buffer or nullptr
   */
  virtual void *TurboBuffer() = 0;

  /**
   * Print board specific information to stdout
   * Called periodically from a timer
   */
  virtual void DebugInfo() = 0;

  /**
   * Check if connected to a USB Host
   * @return true if connected
   */
  virtual bool UsbConnected() = 0;

#ifdef ESP_PLATFORM
  /**
   * Register a callback for USB Connect/Disconnect.
   * If the callback had been already registered, it will be overwritten
   * @param callback
   * @return 0 if successful
   */
  virtual int UsbCallBack(tusb_msc_callback_t callback) = 0;
#endif

  /**
   * The name of current board
   * @return null terminated string
   */
  virtual const char * Name() = 0;

  /**
   * The running software version
   * @return null terminated string
   */
  virtual const char *SwVersion() = 0;

  /**
   * The device serial number
   * @return null terminated string
   */
  virtual char *SerialNumber() = 0;


  // OTA Stuff
  /**
   * Print information to stdout.
   * E.G. Current boot partition
   */
  virtual void BootInfo() {};
  /**
   * Check if an OTA update image file exists on the storage media
   * @return true if one exists
   */
  virtual bool OtaCheck() { return false;};
  /**
   * Validates the OTA update file
   * @return OtaError
   */
  virtual OtaError OtaValidate() { return OtaError::OK; };
  /**
   * Start installing the OTA update
   */
  virtual void OtaInstall() {};
  /**
   * Get the current progress of the OTA update
   * @return 0 to 100%
   */
  virtual int OtaStatus() {return 100;};
/**
 * Get the current state of the OTA update
 * @return 0 to 100%
 */
  virtual OtaError OtaState() {return OtaError::OK;};

/**
* Get the "friendly" error message
* @return 0 to 100%
*/
  virtual const char * OtaString() {return "";};



  /**
   * The source of the wakeup. E.G. power button, hw reset
   * @return
   */
  virtual WakeupSource BootReason() {return WakeupSource::KEY;};

  /**
 * The amount of the memory available for use on this board
 * @return amount of memory in bytes
 */
  virtual size_t MemorySize() { return 4*1024*1024;};

};
}


