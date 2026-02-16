/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include <esp_lcd_panel_ops.h>
#include "log.h"
#include <driver/spi_master.h>
#include <esp_lcd_panel_commands.h>
#include <FreeRTOS.h>
#include <task.h>
#include <esp_lcd_panel_rgb.h>
#include <esp_timer.h>
#include <esp_lcd_panel_io.h>
#include <cstring>
#include "drivers/display_st7701s.h"
#include "esp_lcd_panel_io_additions.h"

static const char *TAG = "display_st7701s.cpp";

/**
 * @brief LCD panel initialization commands.
 *
 */
typedef struct {
  int cmd;                /*<! The specific LCD command */
  const void *data;       /*<! Buffer that holds the command specific data */
  size_t data_bytes;      /*<! Size of `data` in memory, in bytes */
  unsigned int delay_ms;  /*<! Delay in milliseconds after this command */
} st7701_lcd_init_cmd_t;

static const st7701_lcd_init_cmd_t buyadisplaycom[] = {
    {0xFF, (uint8_t[]) {0x77, 0x01, 0x00, 0x00, 0x10,}, 5, 0},
    {0xC0, (uint8_t[]) {0x3b, 0x00,}, 2, 0},
    {0xC1, (uint8_t[]) {0x0b, 0x02,}, 2, 0},
    {0xC2, (uint8_t[]) {0x07, 0x02,}, 2, 0},
    {0xCC, (uint8_t[]) {0x10,}, 1, 0},
    {0xCD, (uint8_t[]) {0x08,}, 1, 0},
    {0xB0, (uint8_t[]) {0x00, 0x11, 0x16, 0x0E, 0x11, 0x06, 0x05, 0x09, 0x08, 0x21, 0x06, 0x13, 0x10, 0x29, 0x31,
                        0x18}, 16, 0},
    {0xB1, (uint8_t[]) {0x00, 0x11, 0x16, 0x0E, 0x11, 0x07, 0x05, 0x09, 0x09, 0x21, 0x05, 0x13, 0x11, 0x2A, 0x31,
                        0x18}, 16, 0},
    {0xFF, (uint8_t[]) {0x77, 0x01, 0x00, 0x00, 0x11,}, 5, 0},
    {0xB0, (uint8_t[]) {0x6D}, 1, 0},
    {0xB1, (uint8_t[]) {0x37}, 1, 0},
    {0xB2, (uint8_t[]) {0x81}, 1, 0},
    {0xB3, (uint8_t[]) {0x80}, 1, 0},
    {0xB5, (uint8_t[]) {0x43}, 1, 0},
    {0xB7, (uint8_t[]) {0x85}, 1, 0},
    {0xB8, (uint8_t[]) {0x20}, 1, 0},
    {0xC1, (uint8_t[]) {0x78}, 1, 0},
    {0xC2, (uint8_t[]) {0x78}, 1, 0},

    {0xC3, (uint8_t[]) {0x8C}, 1, 0},
    {0xD0, (uint8_t[]) {0x88}, 1, 0},
    {0xE0, (uint8_t[]) {0x00, 0x00, 0x02}, 3, 0},
    {0xE1, (uint8_t[]) {0x03, 0xA0, 0x00, 0x00, 0x04}, 5, 0},
    {0xA0, (uint8_t[]) {0x00, 0x00, 0x00, 0x20, 0x20}, 5, 0},
    {0xE2, (uint8_t[]) {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,}, 13, 0},
    {0xE3, (uint8_t[]) {0x00, 0x00, 0x11, 0x00}, 4, 0},
    {0xE4, (uint8_t[]) {0x22, 0x00}, 2, 0},
    {0xE5, (uint8_t[]) {0x05, 0xEC, 0xA0, 0xA0, 0x07, 0xEE, 0xA0, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00}, 16, 0},
    {0xE6, (uint8_t[]) {0x00, 0x00, 0x11, 0x00}, 4, 0},
    {0xE7, (uint8_t[]) {0x22, 0x00}, 2, 0},
    {0xE8, (uint8_t[]) {0x06, 0xED, 0xA0, 0xA0, 0x08, 0xEF, 0xA0, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00}, 16, 0},
    {0xEB, (uint8_t[]) {0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00}, 7, 0},
    {0xED, (uint8_t[]) {0xFF, 0xFF, 0xFF, 0xBA, 0x0A, 0xBF, 0x45, 0xFF, 0xFF, 0x54, 0xFB, 0xA0, 0xAB, 0xFF,
                        0xFF}, 15, 0},
    {0xEF, (uint8_t[]) {0x10, 0x0D, 0x04, 0x08, 0x3F, 0x1F}, 6, 0},
    {0xFF, (uint8_t[]) {0x77, 0x01, 0x00, 0x00, 0x13}, 5, 0},
    {0xEF, (uint8_t[]) {0x08}, 1, 0},
    {0xFF, (uint8_t[]) {0x77, 0x01, 0x00, 0x00, 0x00}, 5, 0},
    {0x11, (uint8_t[]) {}, 0, 120},
    {0x29, (uint8_t[]) {}, 0, 0},
//        {0x23, (uint8_t[]) {},                                                                              0,  0},

};

static void flushTimer(void *args){
  auto callback = static_cast<esp_lcd_panel_io_color_trans_done_cb_t *>(args);
  if(*callback){
    (*callback)(nullptr, nullptr, nullptr);
  }

}

hal::display::esp32s3::display_st7701s::display_st7701s(spi_line_config_t line_cfg,
                                                        int hsync,
                                                        int vsync,
                                                        int de,
                                                        int pclk,
                                                        std::array<int, 16> &rgb) {

  LOGI(TAG, "Install 3-wire SPI panel IO");
  esp_lcd_panel_io_3wire_spi_config_t io_config = {
      .line_config = line_cfg,
      .expect_clk_speed = 100 * 1000,
      .spi_mode = 0,
      .lcd_cmd_bytes = 1,
      .lcd_param_bytes = 1,
      .flags = {
          .use_dc_bit = 1,
          .dc_zero_on_data = 0,
          .lsb_first = 0,
          .cs_high_active = 0,
          .del_keep_cs_inactive = 1,
      },
  };

  esp_lcd_panel_io_handle_t io;

  ESP_ERROR_CHECK(esp_lcd_new_panel_io_3wire_spi(&io_config, &io));

  ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io, 0xf0, (const uint8_t[]) {0x77, 0x01, 0x00, 0x00, 0x00}, 1));

  // Set color format
  // ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL, (const uint8_t[]) {0x00, LCD_CMD_BGR_BIT}, 2));

  ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io, LCD_CMD_COLMOD, (const uint8_t[]) {0x60}, 1));

  // vendor specific initialization, it can be different between manufacturers
  // should consult the LCD supplier for initialization sequence code
  const st7701_lcd_init_cmd_t *init_cmds = buyadisplaycom;
  uint16_t init_cmds_size = sizeof(buyadisplaycom) / sizeof(st7701_lcd_init_cmd_t);

  for (int i = 0; i < init_cmds_size; i++) {
    if(esp_lcd_panel_io_tx_param(io, init_cmds[i].cmd, init_cmds[i].data, init_cmds[i].data_bytes) != ESP_OK){
      LOGI(TAG, "Error Writing LCD CMD %d", i);
      break;
    }
    vTaskDelay(pdMS_TO_TICKS(init_cmds[i].delay_ms));
  }
  ESP_LOGD(TAG, "send init commands success");

  esp_lcd_panel_io_del(io);

  LOGI(TAG, "Install ST7701S panel driver");
  esp_lcd_rgb_panel_config_t rgb_config = {
//            .clk_src = LCD_CLK_SRC_XTAL,
      .clk_src = LCD_CLK_SRC_DEFAULT,
      .timings = {
          .pclk_hz = 7 * 1000 * 1000,
          .h_res = 480,
          .v_res = 480,
          .hsync_pulse_width = 6,
          .hsync_back_porch = 18,
          .hsync_front_porch = 24,
          .vsync_pulse_width = 4,
          .vsync_back_porch = 10,
          .vsync_front_porch = 16,
          .flags {
              .hsync_idle_low = 0,
              .vsync_idle_low = 0,
              .de_idle_high = 0,
              .pclk_active_neg = false,
              .pclk_idle_high = false,}
      },

      .data_width = 16,
      // .bits_per_pixel = 16,
      .num_fbs = fb_number,
      .bounce_buffer_size_px = 0, //10*H_RES,
      // .sram_trans_align = 0,
      // .psram_trans_align = 64,
      .hsync_gpio_num = static_cast<gpio_num_t>(hsync),
      .vsync_gpio_num = static_cast<gpio_num_t>(vsync),
      .de_gpio_num = static_cast<gpio_num_t>(de),
      .pclk_gpio_num = static_cast<gpio_num_t>(pclk),
      .disp_gpio_num = GPIO_NUM_NC,
//            .data_gpio_nums = { B1, B2, B3, B4, B5, G0, G1, G2, G3, G4, G5, R1, R2, R3, R4, R5}, //Working BE
//            .data_gpio_nums = {G3, G4, G5, R1, R2, R3, R4, R5, B1, B2, B3, B4, B5, G0, G1, G2}, //Working LE
      .data_gpio_nums = {static_cast<gpio_num_t>(rgb[0]), static_cast<gpio_num_t>(rgb[1]), static_cast<gpio_num_t>(rgb[2]), static_cast<gpio_num_t>(rgb[3]), static_cast<gpio_num_t>(rgb[4]), static_cast<gpio_num_t>(rgb[5]), static_cast<gpio_num_t>(rgb[6]), static_cast<gpio_num_t>(rgb[7]), static_cast<gpio_num_t>(rgb[8]), static_cast<gpio_num_t>(rgb[9]), static_cast<gpio_num_t>(rgb[10]),
                         static_cast<gpio_num_t>(rgb[11]), static_cast<gpio_num_t>(rgb[12]), static_cast<gpio_num_t>(rgb[13]), static_cast<gpio_num_t>(rgb[14]), static_cast<gpio_num_t>(rgb[15])},

      .flags = {
          .disp_active_low = 0,
          .refresh_on_demand = 0,
          .fb_in_psram = 1,
          .double_fb = 0,
          .no_fb = 0,
          .bb_invalidate_cache = 0
      }
  };

  ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&rgb_config, &panel_handle));
  ESP_LOGD(TAG, "new RGB panel @%p", panel_handle);
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
  esp_lcd_rgb_panel_get_frame_buffer(panel_handle, fb_number, &_fb0, &_fb1);
  size = {480, 480};
  buffer = static_cast<uint8_t *>(_fb0);

  const esp_timer_create_args_t flushTimerArgs = {
      .callback = &flushTimer,
      .arg = &flushCallback,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "st7701s flush timer",
      .skip_unhandled_events = true
  };
  ESP_ERROR_CHECK(esp_timer_create(&flushTimerArgs, &flushTimerHandle));
}

void hal::display::esp32s3::display_st7701s::write(int x_start, int y_start, int x_end, int y_end, void *color_data) {
  buffer = static_cast<uint8_t *>(color_data == _fb0 ? _fb1 : _fb0);
  esp_lcd_panel_draw_bitmap(panel_handle, x_start, y_start, x_end, y_end, color_data);
  esp_timer_start_once(flushTimerHandle, 30*1000);
  if(_clear){
    _clear = false;
    memset(buffer, 0x00, size.first * size.second * 2);
  }
}

bool hal::display::esp32s3::display_st7701s::onColorTransDone(flushCallback_t callback) {
  flushCallback = callback;
  return true;
}

void hal::display::esp32s3::display_st7701s::clear() {
  _clear = true;
  memset(buffer, 0x00, size.first * size.second * 2);
}


