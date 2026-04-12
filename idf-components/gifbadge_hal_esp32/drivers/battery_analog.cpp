/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include <esp_adc/adc_oneshot.h>
#include "log.h"
#include <esp_timer.h>
#include <valarray>
#include "drivers/battery_analog.h"

static const char *TAG = "battery_analog.cpp";

//https://github.com/rlogiacco/BatterySense/blob/f43f782be47c3b6b7fd8a9ae4dfee7981aec4627/Battery.h#L101
static inline uint8_t sigmoidal(const uint16_t voltage, uint16_t minVoltage, uint16_t maxVoltage) {
  const auto result =
      static_cast<uint8_t>(105 - (105 / (1 + pow(1.724 * (voltage - minVoltage) / (maxVoltage - minVoltage), 5.5))));
  return result >= 100 ? 100 : result;
}

hal::battery::esp32s3::battery_analog::battery_analog(adc_channel_t) {
  adc_oneshot_unit_init_cfg_t
      init_config1 = {.unit_id = ADC_UNIT_1, .clk_src = ADC_RTC_CLK_SRC_DEFAULT, .ulp_mode = ADC_ULP_MODE_DISABLE,};
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc_handle));
  adc_oneshot_chan_cfg_t config = {.atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_DEFAULT,};
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_9, &config));
  LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
  adc_cali_curve_fitting_config_t cali_config =
      {.unit_id = ADC_UNIT_1, .chan = ADC_CHANNEL_9, .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_DEFAULT,};
  ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cali_config, &calibration_scheme));
  int reading;
  int voltage;
  adc_oneshot_read(adc_handle, ADC_CHANNEL_9, &reading);
  ESP_ERROR_CHECK(adc_cali_raw_to_voltage(calibration_scheme, reading, &voltage));
  smoothed_voltage = voltage / 1000.00;
  LOGI(TAG, "Initial Voltage: %f", smoothed_voltage * 2);
  alpha = 0.05;
  const esp_timer_create_args_t battery_timer_args = {.callback = [](void *params) {
    const auto bat = static_cast<battery_analog *>(params);
    bat->poll();
  }, .arg = this, .dispatch_method = ESP_TIMER_TASK, .name = "battery_analog", .skip_unhandled_events = true};
  esp_timer_handle_t battery_handler_handle = nullptr;
  ESP_ERROR_CHECK(esp_timer_create(&battery_timer_args, &battery_handler_handle));
  esp_timer_start_periodic(battery_handler_handle, 250 * 1000);
}

hal::battery::esp32s3::battery_analog::~battery_analog() = default;

void hal::battery::esp32s3::battery_analog::poll() {
  int reading;
  int voltage;
  adc_oneshot_read(adc_handle, ADC_CHANNEL_9, &reading);
  ESP_ERROR_CHECK(adc_cali_raw_to_voltage(calibration_scheme, reading, &voltage));
  smoothed_voltage = (voltage * scale_factor) * (alpha) + smoothed_voltage * (1.0f - alpha);
}
double hal::battery::esp32s3::battery_analog::BatteryVoltage() {
  return smoothed_voltage / 1000.00;
}
int hal::battery::esp32s3::battery_analog::BatterySoc() {
  return sigmoidal(static_cast<uint16_t>(smoothed_voltage), 3000, 4200);
}

