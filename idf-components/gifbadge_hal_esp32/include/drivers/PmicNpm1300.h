/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once

#include <memory>

#include <hal/battery.h>
#include <esp_timer.h>
#include <driver/i2c_types.h>
#include "hal/keys.h"
#include "npmx_backend.h"
#include "npmx_instance.h"
#include "hal/gpio.h"
#include "hal/charger.h"
#include "hal/board.h"
#include "hal/vbus.h"

namespace hal::pmic::esp32s3 {

class PmicNpm1300;

class PmicNpm1300Gpio final : public gpio::Gpio {
 public:
  PmicNpm1300Gpio(npmx_instance_t *npmx, uint8_t index);
  ~PmicNpm1300Gpio() = default;
  void GpioConfig(gpio::GpioDirection direction, gpio::GpioPullMode pull) override;
  gpio::GpioState GpioRead() override;
  void GpioWrite(bool b) override;
  void EnableIrq(bool b);
  void GpioInt(hal::gpio::GpioIntDirection dir, void (*callback)()) override;

 private:
  npmx_instance_t *_npmx_instance;
  npmx_gpio_config_t _config{
      .mode = NPMX_GPIO_MODE_INPUT,
      .drive = NPMX_GPIO_DRIVE_6_MA,
      .pull = NPMX_GPIO_PULL_DOWN,
      .open_drain = false,
      .debounce = false,
  };
  uint8_t _index;
  gpio::GpioIntDirection _int_direction = gpio::GpioIntDirection::NONE;
  void (*_callback)() = nullptr;
};

class PmicNpm1300ShpHld : public gpio::Gpio {
 public:
  explicit PmicNpm1300ShpHld(npmx_instance_t *npmx);
  void GpioConfig(gpio::GpioDirection direction, gpio::GpioPullMode pull) override;
  gpio::GpioState GpioRead() override;
  void GpioWrite(bool b) override;

 private:
  npmx_instance_t *_npmx_instance;
};

class PmicNpm1300Led : public gpio::Gpio {
 public:
  PmicNpm1300Led(npmx_instance_t *npmx, uint8_t index);
  void GpioConfig(gpio::GpioDirection direction, gpio::GpioPullMode pull) override;
  gpio::GpioState GpioRead() override;
  void GpioWrite(bool b) override;
  void ChargingIndicator(bool b);

 private:
  npmx_instance_t *_npmx_instance;
  uint8_t _index;
};

class PmicNpm1300 final : public hal::battery::Battery, public hal::charger::Charger, public hal::vbus::Vbus {
 public:
  explicit PmicNpm1300(i2c_master_bus_handle_t i2c, gpio_num_t gpio_int);
  ~PmicNpm1300() final = default;

  void Init();

  //Buck regulators
  void Buck1Set(uint32_t millivolts);
  void Buck1Disable();

  //Load Switch
  void LoadSw1Enable();
  void LoadSw1Disable();

  //GPIO
  PmicNpm1300Gpio *GpioGet(uint8_t index);
  PmicNpm1300ShpHld *ShphldGet();
  PmicNpm1300Led *LedGet(uint8_t index);

  void poll();

  double BatteryVoltage() override;
  double BatteryCurrent() override;
  double BatteryTemperature() override;

  int BatterySoc() override;

  void BatteryRemoved() override;

  void BatteryInserted() override;

  State BatteryStatus() override;

  void ChargeEnable() override;
  void ChargeDisable() override;
  void ChargeCurrentSet(uint16_t iset) override;
  uint16_t ChargeCurrentGet() override;
  void DischargeCurrentSet(uint16_t iset) override;
  uint16_t DischargeCurrentGet() override;
  void ChargeVtermSet(uint16_t vterm) override;
  uint16_t ChargeVtermGet() override;
  ChargeStatus ChargeStatusGet() override;
  ChargeError ChargeErrorGet() override;
  bool ChargeBattDetect() override;

  void HandleInterrupt();
  void Loop();

  Boards::WakeupSource GetWakeup();

  void PwrLedSet(hal::gpio::Gpio *gpio);

  void EnableGpioEvent(uint8_t index);
  void DisableGpioEvent(uint8_t index);
  void RegisterGpioCallback(uint8_t index, void (*callback)());

  void EnableADC();

  uint16_t VbusMaxCurrentGet() override;
  void VbusMaxCurrentSet(uint16_t mA) override;
  bool VbusConnected() override;
  void VbusConnectedCallback(void (*callback)(bool));

  void DebugLog();

 private:
  i2c_master_dev_handle_t i2c_handle = nullptr;
  double _voltage = 0;
  int _soc = 0;
  double _rate = 0;
  gpio_num_t _gpio_int;
  bool _present = false;
  Boards::WakeupSource _wakeup_source = Boards::WakeupSource::NONE;
  hal::gpio::Gpio *_power_led = nullptr;

//NPM1300 Stuff
  npmx_backend_t _npmx_backend;
  npmx_instance_t _npmx_instance;

  ChargeError _charge_error = ChargeError::NONE;

  esp_timer_handle_t _looptimer = nullptr;

  static void VbusVoltage(npmx_instance_t *pm, npmx_callback_type_t type, uint8_t mask);
  void (*_vbus_callback)(bool) = nullptr;
  static void GpioHandler(npmx_instance_t *pm, npmx_callback_type_t type, uint8_t mask);
  void GpioCallback(uint8_t index);

  uint8_t _gpio_event_mask;
  void (*_gpio_callbacks[5])() = {nullptr, nullptr, nullptr, nullptr, nullptr};

  esp_timer_handle_t _adc_timer = nullptr;
  static void ADCTimerHandler(void *arg);
  int32_t _vbat;
  int32_t _ibat;
  int32_t _tbat;


};

}
