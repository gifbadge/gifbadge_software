/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include <driver/spi_master.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_ops.h>
#include <driver/gpio.h>
#include <esp_lcd_panel_io.h>
#include <cstring>
#include "log.h"
#include "esp_lcd_gc9a01.h"

#include "drivers/display_gc9a01.h"

#include <esp_heap_caps.h>

static const char *TAG = "display_gc9a01.cpp";

hal::display::esp32s3::display_gc9a01::display_gc9a01(int mosi, int sck, int cs, int dc, int reset) {
  LOGI(TAG, "Initialize SPI bus");
  spi_bus_config_t buscfg = {
      .mosi_io_num = mosi,//21,
      .miso_io_num = -1,
      .sclk_io_num = sck,//40,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .data4_io_num = -1,
      .data5_io_num = -1,
      .data6_io_num = -1,
      .data7_io_num = -1,
      .max_transfer_sz = 16384, //H_RES * V_RES * sizeof(uint16_t),
      .flags = 0,
      .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
      .intr_flags = 0,
  };
  ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

  LOGI(TAG, "Install panel IO");
  esp_lcd_panel_io_spi_config_t io_config = {
      .cs_gpio_num = static_cast<gpio_num_t>(cs), //47,
      .dc_gpio_num = static_cast<gpio_num_t>(dc),//45,
      .spi_mode = 0,
      .pclk_hz = (80 * 1000 * 1000),
      .trans_queue_depth = 10,
      .on_color_trans_done = nullptr,
      .user_ctx = nullptr,
      .lcd_cmd_bits = 8,
      .lcd_param_bits = 8,
      .flags = {
          .dc_high_on_cmd = 0,
          .dc_low_on_data = 0,
          .dc_low_on_param = 0,
          .octal_mode = 0,
          .quad_mode = 0,
          .sio_mode = 0,
          .lsb_first = 0,
          .cs_high_active = 0,
      }
  };
  // Attach the LCD to the SPI bus
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t) SPI2_HOST, &io_config, &io_handle));

  esp_lcd_panel_dev_config_t panel_config = {
      // .rgb_endian = LCD_RGB_ENDIAN_BGR,
      .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
      .data_endian = LCD_RGB_DATA_ENDIAN_BIG,
      .bits_per_pixel = 16,
      .reset_gpio_num = static_cast<gpio_num_t>(reset),
      .vendor_config = nullptr,
      .flags = {
          .reset_active_high = 0,
      },
  };

  LOGI(TAG, "Install GC9A01 panel driver");
  ESP_ERROR_CHECK(esp_lcd_new_panel_gc9a01(io_handle, &panel_config, &panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
  ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
  gpio_hold_en(static_cast<gpio_num_t>(reset)); //Don't toggle the reset signal on light sleep
  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
  buffer = static_cast<uint8_t *>(heap_caps_malloc(240 * 240 * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  size = {240, 240};
}

static flushCallback_t pcallback = nullptr;

bool flush_ready(esp_lcd_panel_io_handle_t, esp_lcd_panel_io_event_data_t *, void *){
//  LOGI(TAG, "Flush");
  if(pcallback) {
    pcallback();
  }
  return false;
}

bool hal::display::esp32s3::display_gc9a01::onColorTransDone(flushCallback_t callback) {
  if (callback) {
    pcallback = callback;
    esp_lcd_panel_io_callbacks_t conf = {.on_color_trans_done = flush_ready};
    esp_lcd_panel_io_register_event_callbacks(io_handle, &conf, nullptr);
  } else {
    pcallback = nullptr;
    esp_lcd_panel_io_callbacks_t conf = {.on_color_trans_done = nullptr};
    esp_lcd_panel_io_register_event_callbacks(io_handle, &conf, nullptr);
  }
  return true;
}


void lv_draw_sw_rgb565_swap(void * buf, uint32_t buf_size_px)
{
  //Adapted from lvgl src/draw/sw/lv_draw_sw_utils.c
  auto * buf16 = static_cast<uint16_t *>(buf);

  /*2 pixels will be processed later, so handle 1 pixel alignment*/
  if(reinterpret_cast<uintptr_t>(buf16) & 0x2) {
    buf16[0] = ((buf16[0] & 0xff00) >> 8) | ((buf16[0] & 0x00ff) << 8);
    buf16++;
    buf_size_px--;
  }

  auto * buf32 = reinterpret_cast<uint32_t *>(buf16);
  uint32_t u32_cnt = buf_size_px / 2;

  while(u32_cnt >= 8) {
    buf32[0] = ((buf32[0] & 0xff00ff00) >> 8) | ((buf32[0] & 0x00ff00ff) << 8);
    buf32[1] = ((buf32[1] & 0xff00ff00) >> 8) | ((buf32[1] & 0x00ff00ff) << 8);
    buf32[2] = ((buf32[2] & 0xff00ff00) >> 8) | ((buf32[2] & 0x00ff00ff) << 8);
    buf32[3] = ((buf32[3] & 0xff00ff00) >> 8) | ((buf32[3] & 0x00ff00ff) << 8);
    buf32[4] = ((buf32[4] & 0xff00ff00) >> 8) | ((buf32[4] & 0x00ff00ff) << 8);
    buf32[5] = ((buf32[5] & 0xff00ff00) >> 8) | ((buf32[5] & 0x00ff00ff) << 8);
    buf32[6] = ((buf32[6] & 0xff00ff00) >> 8) | ((buf32[6] & 0x00ff00ff) << 8);
    buf32[7] = ((buf32[7] & 0xff00ff00) >> 8) | ((buf32[7] & 0x00ff00ff) << 8);
    buf32 += 8;
    u32_cnt -= 8;
  }

  while(u32_cnt) {
    *buf32 = ((*buf32 & 0xff00ff00) >> 8) | ((*buf32 & 0x00ff00ff) << 8);
    buf32++;
    u32_cnt--;
  }

  /*Process the last pixel if needed*/
  if(buf_size_px & 0x1) {
    uint32_t e = buf_size_px - 1;
    buf16[e] = ((buf16[e] & 0xff00) >> 8) | ((buf16[e] & 0x00ff) << 8);
  }

}

void hal::display::esp32s3::display_gc9a01::write(int x_start,
                                                  int y_start,
                                                  int x_end,
                                                  int y_end,
                                                  void *color_data) {
  lv_draw_sw_rgb565_swap(color_data, 240*240);
  esp_lcd_panel_draw_bitmap(panel_handle, x_start, y_start, x_end, y_end, color_data);
}

void hal::display::esp32s3::display_gc9a01::clear() {
  memset(buffer, 0xFF, size.first * size.second * 2);
}