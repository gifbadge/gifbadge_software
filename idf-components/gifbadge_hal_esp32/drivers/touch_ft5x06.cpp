/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include "drivers/touch_ft5x06.h"

#include <utility>
#include <driver/i2c_master.h>

hal::touch::esp32s3::touch_ft5x06::touch_ft5x06(i2c_master_bus_handle_t i2c) {
  uint8_t out[2];

  i2c_device_config_t dev_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = 0x38,
    .scl_speed_hz = 400000,
    .flags = {.disable_ack_check = true},
  };
  ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c, &dev_cfg, &i2c_handle));

  // Valid touching detect threshold
  out[0] = FT5x06_ID_G_THGROUP;
  out[1] = 70;
  i2c_master_transmit(i2c_handle, out, 2 , 100);

  // valid touching peak detect threshold
  out[0] = FT5x06_ID_G_THPEAK;
  out[1] = 60;
  i2c_master_transmit(i2c_handle, out, 2 , 100);

  // Touch focus threshold
  out[0] = FT5x06_ID_G_THCAL;
  out[1] = 16;
  i2c_master_transmit(i2c_handle, out, 2 , 100);

  // threshold when there is surface water
  out[0] = FT5x06_ID_G_THWATER;
  out[1] = 60;
  i2c_master_transmit(i2c_handle, out, 2 , 100);

  // threshold of temperature compensation
  out[0] = FT5x06_ID_G_THTEMP;
  out[1] = 10;
  i2c_master_transmit(i2c_handle, out, 2 , 100);

  // Touch difference threshold
  out[0] = FT5x06_ID_G_THDIFF;
  out[1] = 20;
  i2c_master_transmit(i2c_handle, out, 2 , 100);

  // Delay to enter 'Monitor' status (s)
  out[0] = FT5x06_ID_G_TIME_ENTER_MONITOR;
  out[1] = 2;
  i2c_master_transmit(i2c_handle, out, 2 , 100);

  // Period of 'Active' status (ms)
  out[0] = FT5x06_ID_G_PERIODACTIVE;
  out[1] = 12;
  i2c_master_transmit(i2c_handle, out, 2 , 100);

  // Timer to enter 'idle' when in 'Monitor' (ms)
  out[0] = FT5x06_ID_G_PERIODMONITOR;
  out[1] = 40;
  i2c_master_transmit(i2c_handle, out, 2 , 100);
}

hal::touch::touch_data hal::touch::esp32s3::touch_ft5x06::read() {
  uint8_t data_in = 0;
  uint8_t data_out = FT5x06_TOUCH_POINTS;
  esp_err_t ret = i2c_master_transmit_receive(i2c_handle, &data_out, 1, &data_in, 1, 10);
  if (ret == ESP_OK) {
    if ((data_in & 0x0f) > 0 && (data_in & 0x0f) < 5) {
      uint8_t tmp[4] = {0};
      data_out = FT5x06_TOUCH1_XH;
      i2c_master_transmit_receive(i2c_handle, &data_out, 1, tmp, 4, 100);
      auto x = static_cast<int16_t>(((tmp[0] & 0x0f) << 8) + tmp[1]); //not rotated
      auto y = static_cast<int16_t>(((tmp[2] & 0x0f) << 8) + tmp[3]);
      if (x > 0 || y > 0) {
        return {.state = TOUCH_PRESS, .position = {x, y}};
      }
    }
    return{.state = TOUCH_RELEASE, .position = {0, 0}};
  }
  return{.state = TOUCH_INVALID, .position = {0, 0}};
}



