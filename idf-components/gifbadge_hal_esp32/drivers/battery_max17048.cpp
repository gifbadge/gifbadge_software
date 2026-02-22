/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include "drivers/battery_max17048.h"

#include "log.h"
#include <esp_timer.h>
#include <driver/gpio.h>
#include <driver/i2c_master.h>

static const char *TAG = "battery_max17048";

hal::battery::esp32s3::battery_max17048::battery_max17048(i2c_master_bus_handle_t i2c, gpio_num_t vbus_pin)
    : i2c_master(i2c), _vbus_pin(vbus_pin) {
  i2c_device_config_t dev_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = 0x36,
    .scl_speed_hz = 400000,
    .scl_wait_us = 0,
    .flags = {.disable_ack_check = true} //MAX17048 may be powered off of battery VDD, and may be unstable, resulting in NAKs
  };
  ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c, &dev_cfg, &i2c_handle));
  uint8_t data_in[2];
  uint8_t data_out = 0x08;
  if (i2c_master_transmit_receive(i2c_handle, &data_out, 1 ,data_in, 2, 100) == ESP_OK) {
    LOGI(TAG, "MAX17048 Version %u %u", data_in[0], data_in[1]);
  } else {
    LOGI(TAG, "MAX17048 communication error", data_in[0], data_in[1]);
  }
  const esp_timer_create_args_t battery_timer_args = {
      .callback = [](void *params) {
        auto bat = static_cast<battery_max17048 *>(params);
        bat->poll();
      },
      .arg = this,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "battery_max17048",
      .skip_unhandled_events = true
  };
  esp_timer_handle_t battery_handler_handle = nullptr;
  ESP_ERROR_CHECK(esp_timer_create(&battery_timer_args, &battery_handler_handle));
  esp_timer_start_periodic(battery_handler_handle, 10000 * 1000);
  poll();
}

void hal::battery::esp32s3::battery_max17048::poll() {
  uint8_t data_in[2];
  uint8_t data_out = 0x02;
  i2c_master_transmit_receive(i2c_handle, &data_out, 1 ,data_in, 2, 100);
  _voltage = ((static_cast<uint16_t>(data_in[0] << 8) | data_in[1]) * 78.125) / 1000000;
  data_out = 0x04;
  i2c_master_transmit_receive(i2c_handle, &data_out, 1 ,data_in, 2, 100);
  _soc = static_cast<int>((static_cast<uint16_t>(data_in[0] << 8) | data_in[1]) / 256.00);
  data_out = 0x16;
  i2c_master_transmit_receive(i2c_handle, &data_out, 1 ,data_in, 2, 100);
  _rate = (static_cast<int16_t>(data_in[0] << 8) | data_in[1]) * 0.208;
}

double hal::battery::esp32s3::battery_max17048::BatteryVoltage() {
  return _voltage;
}

int hal::battery::esp32s3::battery_max17048::BatterySoc() {
  return _soc;
}

void hal::battery::esp32s3::battery_max17048::BatteryRemoved() {
  present = false;
}

void hal::battery::esp32s3::battery_max17048::BatteryInserted() {
  // Quickstart. so the MAX17048 restarts it's SOC algorythm.
  //Prevents erroneous readings if battery is swapped while charging
  uint8_t cmd[] = {0x06, 0x80, 0x00};
  i2c_master_transmit(i2c_handle, cmd, sizeof(cmd) , 100);
  present = true;
}

hal::battery::Battery::State hal::battery::esp32s3::battery_max17048::BatteryStatus() {
  if(!present){
    return State::NOT_PRESENT;
  }
  if(gpio_get_level(_vbus_pin) && _rate > 1){
    return State::CHARGING;
  }
  if(gpio_get_level(_vbus_pin)){
    return State::CONNECTED_NOT_CHARGING;
  }
  if(!gpio_get_level(_vbus_pin)) {
    return State::DISCHARGING;
  }
  return hal::battery::Battery::State::ERROR;
}
