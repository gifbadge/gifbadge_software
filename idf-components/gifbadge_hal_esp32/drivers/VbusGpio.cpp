/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include <driver/gpio.h>
#include "drivers/vbus_gpio.h"

hal::vbus::esp32s3::VbusGpio::VbusGpio(const gpio_num_t gpio): _gpio(gpio) {

}

uint16_t hal::vbus::esp32s3::VbusGpio::VbusMaxCurrentGet() {
  if(gpio_get_level(_gpio)){
    return 500;
  }
  return 0;
}

void hal::vbus::esp32s3::VbusGpio::VbusMaxCurrentSet(uint16_t mA) {

}
bool hal::vbus::esp32s3::VbusGpio::VbusConnected() {
  return gpio_get_level(_gpio);
}
