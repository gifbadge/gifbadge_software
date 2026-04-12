/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include <cinttypes>
#include <cstdlib>

#include "esp_check.h"
#include "esp_log.h"

#include "esp_io_expander.h"
#include "drivers/esp_io_expander_gpio.h"
#include "hal/gpio.h"

#define IO_COUNT                (1)



/* Default register value on power-up */
#define DIR_REG_DEFAULT_VAL     (0xffff)
#define OUT_REG_DEFAULT_VAL     (0x0000)

/**
 * @brief Device Structure Type
 *
 */
typedef struct {
  esp_io_expander_t base;
  hal::gpio::Gpio *gpio;
  struct {
    uint16_t direction;
    uint16_t output;
  } regs;
} esp_io_expander_gpio_t;

static const char *TAG = "esp_io_expander_gpio";

static esp_err_t read_input_reg(esp_io_expander_handle_t handle, uint32_t *value);
static esp_err_t write_output_reg(esp_io_expander_handle_t handle, uint32_t value);
static esp_err_t read_output_reg(esp_io_expander_handle_t handle, uint32_t *value);
static esp_err_t write_direction_reg(esp_io_expander_handle_t handle, uint32_t value);
static esp_err_t read_direction_reg(esp_io_expander_handle_t handle, uint32_t *value);
static esp_err_t reset(esp_io_expander_t *handle);
static esp_err_t del(esp_io_expander_t *handle);

esp_err_t esp_io_expander_new_gpio(hal::gpio::Gpio *gpio, esp_io_expander_handle_t *handle)
{
  ESP_RETURN_ON_FALSE(gpio != nullptr, ESP_ERR_INVALID_ARG, TAG, "Invalid gpio");
  ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "Invalid handle");

  auto *tca = static_cast<esp_io_expander_gpio_t *>(calloc(1, sizeof(esp_io_expander_gpio_t)));
  assert(tca != nullptr); // to silence linter
  ESP_RETURN_ON_FALSE(tca, ESP_ERR_NO_MEM, TAG, "Malloc failed");

  tca->gpio = gpio;
  tca->base.config.io_count = IO_COUNT;
  tca->base.config.flags.dir_out_bit_zero = 1;
  tca->base.read_input_reg = read_input_reg;
  tca->base.write_output_reg = write_output_reg;
  tca->base.read_output_reg = read_output_reg;
  tca->base.write_direction_reg = write_direction_reg;
  tca->base.read_direction_reg = read_direction_reg;
  tca->base.del = del;
  tca->base.reset = reset;

  esp_err_t ret = ESP_OK;
  /* Reset configuration and register status */
  ESP_GOTO_ON_ERROR(reset(&tca->base), err, TAG, "Reset failed");

  *handle = &tca->base;
  return ESP_OK;
  err:
  free(tca);
  return ret;
}

static esp_err_t read_input_reg(const esp_io_expander_handle_t handle, uint32_t *value)
{
  const auto *tca = __containerof(handle, esp_io_expander_gpio_t, base);
  hal::gpio::GpioState tmp = tca->gpio->GpioRead();
  if (tmp != hal::gpio::GpioState::INVALID) {
    *value = tca->gpio->GpioRead() == hal::gpio::GpioState::HIGH ? 1 : 0;
    return ESP_OK;
  }
  return ESP_ERR_INVALID_STATE;
}

static esp_err_t write_output_reg(esp_io_expander_handle_t handle, const uint32_t value)
{
  auto *tca = __containerof(handle, esp_io_expander_gpio_t, base);
  tca->regs.output = value;
  tca->gpio->GpioWrite(value);
  return ESP_OK;
}

static esp_err_t read_output_reg(esp_io_expander_handle_t handle, uint32_t *value)
{
  const auto *tca = __containerof(handle, esp_io_expander_gpio_t, base);
  *value = tca->regs.output;
  return ESP_OK;
}

static esp_err_t write_direction_reg(esp_io_expander_handle_t handle, const uint32_t value)
{
  auto *tca = __containerof(handle, esp_io_expander_gpio_t, base);
  tca->regs.direction = value;
  return ESP_OK;
}

static esp_err_t read_direction_reg(esp_io_expander_handle_t handle, uint32_t *value)
{
  const auto *tca = __containerof(handle, esp_io_expander_gpio_t, base);
  *value = tca->regs.direction;
  return ESP_OK;
}

static esp_err_t reset(esp_io_expander_t *)
{
  return ESP_OK;
}

static esp_err_t del(esp_io_expander_t *handle)
{
  auto *tca = __containerof(handle, esp_io_expander_gpio_t, base);
  free(tca);
  return ESP_OK;
}
