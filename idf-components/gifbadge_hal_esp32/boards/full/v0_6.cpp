/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include "boards/full/v0_6.h"

#include <esp_pm.h>
#include "log.h"
#include <driver/rtc_io.h>
#include <esp_sleep.h>
#include <driver/i2c_master.h>

#include "drivers/keys_esp_io_expander.h"
#include "drivers/esp_io_expander_gpio.h"
#include "esp_efuse_custom_table.h"


// #define USB_ENABLE

static const char *TAG = "Board::esp32::s3::full::v0_6";

static bool checkSdState(hal::gpio::Gpio *gpio) {
  return gpio->GpioRead() == hal::gpio::GpioState::LOW;
}

namespace Boards {

esp32::s3::full::v0_6::v0_6() {
  _config = new hal::config::esp32s3::Config_NVS(this);
  gpio_install_isr_service(0);

  i2c_master_bus_config_t i2c_mst_config = {
    .i2c_port = I2C_NUM_0,
    .sda_io_num = GPIO_NUM_47,
    .scl_io_num = GPIO_NUM_48,
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .glitch_ignore_cnt = 7,
    .intr_priority = 0,
    .trans_queue_depth = 0,
    .flags = {.enable_internal_pullup = false, .allow_pd = false},
  };
  ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

  // _i2c = new I2C(I2C_NUM_0, 47, 48, 100 * 1000, false);

  //Initialize the PMIC, and start the battery charging
  _pmic = new hal::pmic::esp32s3::PmicNpm1300(bus_handle, GPIO_NUM_21);

  //Enable Battery charging
  _pmic->ChargeDisable();
  _pmic->ChargeCurrentSet(400);
  _pmic->ChargeVtermSet(4200);
  _pmic->ChargeEnable();

  //Enable charging LED
  _charge_led = _pmic->LedGet(1);
  _charge_led->ChargingIndicator(true);

  //Enable IRQ from npm1300
  hal::pmic::esp32s3::PmicNpm1300Gpio *irq = _pmic->GpioGet(2);
  irq->EnableIrq(true);
  delete irq;

  //Set up the power LED
  _vbus_led = _pmic->LedGet(0);
  _vbus_led->ChargingIndicator(false);
  _pmic->PwrLedSet(_vbus_led);

  _pmic->Init();

}

hal::battery::Battery *esp32::s3::full::v0_6::GetBattery() {
  return _pmic;
}

hal::touch::Touch *esp32::s3::full::v0_6::GetTouch() {
  return _touch;
}

hal::keys::Keys *esp32::s3::full::v0_6::GetKeys() {
  return _keys;
}

hal::display::Display *esp32::s3::full::v0_6::GetDisplay() {
  return _display;
}

hal::backlight::Backlight *esp32::s3::full::v0_6::GetBacklight() {
  return _backlight;
}

void esp32::s3::full::v0_6::PowerOff() {
  LOGI(TAG, "Poweroff");
  esp32s3_sdmmc::PowerOff();
  vTaskDelay(500 / portTICK_PERIOD_MS);
  rtc_gpio_pullup_dis(GPIO_NUM_21);
  rtc_gpio_pulldown_dis(GPIO_NUM_21);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_21, 1);
  if(_cs != nullptr){
    _cs->GpioConfig(hal::gpio::GpioDirection::IN, hal::gpio::GpioPullMode::NONE);
  }
  _pmic->LoadSw1Disable();
  _pmic->Buck1Disable();
  esp_deep_sleep_start();
}

BoardPower esp32::s3::full::v0_6::PowerState() {
  if (_pmic->VbusConnected()) {
    return BOARD_POWER_NORMAL;
  }
  if (_pmic->BatterySoc() < 12) {
    if (_pmic->BatterySoc() < 5) {
      return BOARD_POWER_CRITICAL;
    }
    return BOARD_POWER_LOW;

  }
  return BOARD_POWER_NORMAL;
}

bool esp32::s3::full::v0_6::StorageReady() {
  return checkSdState(_card_detect);
}

const char *esp32::s3::full::v0_6::Name() {
  return "2.1\" 0.6-0.7";
}
void esp32::s3::full::v0_6::LateInit() {
  buffer = heap_caps_malloc(480 * 480 + 0x6100, MALLOC_CAP_INTERNAL);
  esp_rom_gpio_connect_in_signal(GPIO_MATRIX_CONST_ZERO_INPUT,
                                 USB_SRP_BVALID_IN_IDX,
                                 false); //Start with USB Disconnected

  _pmic->Buck1Set(3300);

  _backlight = new hal::backlight::esp32s3::backlight_ledc(GPIO_NUM_0, true, 0);
  _touch = new hal::touch::esp32s3::touch_ft5x06(bus_handle);

  _cs = _pmic->GpioGet(1);
  _cs->GpioConfig(hal::gpio::GpioDirection::OUT, hal::gpio::GpioPullMode::NONE);
  _cs->GpioWrite(true);

  esp_io_expander_new_gpio(_cs, &_io_expander);

  spi_line_config_t line_config = {
      .cs_io_type = IO_TYPE_EXPANDER,             // Set to `IO_TYPE_GPIO` if using GPIO, same to below
      .cs_gpio_num = 1,
      .scl_io_type = IO_TYPE_GPIO,
      .scl_gpio_num = 3,
      .sda_io_type = IO_TYPE_GPIO,
      .sda_gpio_num = 4,
      .io_expander = _io_expander,                        // Set to NULL if not using IO expander
  };

  hal::pmic::esp32s3::PmicNpm1300Gpio *up = _pmic->GpioGet(4);
  up->GpioConfig(hal::gpio::GpioDirection::IN, hal::gpio::GpioPullMode::UP);

  hal::pmic::esp32s3::PmicNpm1300Gpio *down = _pmic->GpioGet(3);
  down->GpioConfig(hal::gpio::GpioDirection::IN, hal::gpio::GpioPullMode::UP);

  hal::pmic::esp32s3::PmicNpm1300ShpHld *enter = _pmic->ShphldGet();

  _keys = new hal::keys::esp32s3::KeysGeneric(up, down, enter);

  _pmic->LoadSw1Enable();
  _card_detect = _pmic->GpioGet(0);
  _card_detect->GpioConfig(hal::gpio::GpioDirection::IN, hal::gpio::GpioPullMode::UP);

  std::array<int, 16> rgb = { 3, 4, 5, 6, 7,8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18 };
  _display = new hal::display::esp32s3::display_st7701s(line_config, 2, 1, 45, 46, rgb);


  //TODO: Check if we can use DFS with the RGB LCD
  esp_pm_config_t pm_config = {.max_freq_mhz = 240, .min_freq_mhz = 80, .light_sleep_enable = false};
  esp_pm_configure(&pm_config);

  _pmic->EnableADC();

  if (StorageReady()) {
    _card_detect->GpioInt(hal::gpio::GpioIntDirection::RISING, esp_restart);
    mount(GPIO_NUM_40, GPIO_NUM_41, GPIO_NUM_39, GPIO_NUM_38, GPIO_NUM_44, GPIO_NUM_42, GPIO_NUM_NC, 4, GPIO_NUM_NC);
  } else {
    _card_detect->GpioInt(hal::gpio::GpioIntDirection::FALLING, esp_restart);
  }

  _pmic->VbusConnectedCallback(VbusHandler);


}
WakeupSource esp32::s3::full::v0_6::BootReason() {
  LOGI(TAG, "esp_reset_reason() %i", esp_reset_reason());
  if (_pmic->GetWakeup() != WakeupSource::NONE) {
    return _pmic->GetWakeup();
  }
  if (esp_reset_reason() != ESP_RST_POWERON) {
    return WakeupSource::REBOOT;
  }
  return WakeupSource::NONE;
}
hal::vbus::Vbus *esp32::s3::full::v0_6::GetVbus() {
  return _pmic;
}
hal::charger::Charger *esp32::s3::full::v0_6::GetCharger() {
  return _pmic;
}
void esp32::s3::full::v0_6::DebugInfo() {
  _pmic->DebugLog();
  esp32s3_sdmmc::DebugInfo();
}
char *esp32::s3::full::v0_6::SerialNumber() {
  uint64_t sn;
  esp_efuse_read_field_blob(ESP_EFUSE_KEY1_SERIAL, &sn, 64);
  if (sn == 0x00) {
    esp_efuse_read_field_blob(ESP_EFUSE_KEY0_SERIAL, &sn, 64);
  }
  sprintf(serial, "%s", lltoa(sn, 10));
  return serial;
}
}