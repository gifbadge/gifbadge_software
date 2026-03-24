/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include <esp_pm.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>
#include <wear_levelling.h>

#include "hal/board.h"

#include "boards/mini/v0.h"

#include <esp_flash.h>
#include <esp_flash_spi_init.h>
#include <log.h>
#include <tinyusb.h>
#if CFG_TUD_CDC
#include <tinyusb_cdc_acm.h>
#include <tinyusb_console.h>
#endif

#include "drivers/backlight_ledc.h"
#include "tinyusb_msc.h"

static const char *TAG = "board_v0";

namespace Boards {

esp32::s3::mini::v0::v0() {
  _battery = new hal::battery::esp32s3::battery_analog(ADC_CHANNEL_9);
  gpio_install_isr_service(0);
  _keys = new hal::keys::esp32s3::keys_gpio(GPIO_NUM_43, GPIO_NUM_44, GPIO_NUM_0);
  _display = new hal::display::esp32s3::display_gc9a01(35, 36, 34, 37, 46);
  _backlight = new hal::backlight::esp32s3::backlight_ledc(GPIO_NUM_45, false, 0);
}

hal::battery::Battery *esp32::s3::mini::v0::GetBattery() {
  return _battery;
}

hal::touch::Touch *esp32::s3::mini::v0::GetTouch() {
  return nullptr;
}

hal::keys::Keys *esp32::s3::mini::v0::GetKeys() {
  return _keys;
}

hal::display::Display *esp32::s3::mini::v0::GetDisplay() {
  return _display;
}

hal::backlight::Backlight *esp32::s3::mini::v0::GetBacklight() {
  return _backlight;
}

void esp32::s3::mini::v0::PowerOff() {
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
  rtc_gpio_pullup_en(static_cast<gpio_num_t>(0));
  rtc_gpio_pulldown_dis(static_cast<gpio_num_t>(0));
  esp_sleep_enable_ext0_wakeup(static_cast<gpio_num_t>(0), 0);
  esp_deep_sleep_start();
}

BoardPower esp32::s3::mini::v0::PowerState() {
  return BOARD_POWER_NORMAL;
}

char storage_name[] = "Internal Flash";
StorageInfo esp32::s3::mini::v0::GetStorageInfo() {
  uint64_t total_bytes;
  uint64_t free_bytes;
  esp_vfs_fat_info("/data", &total_bytes, &free_bytes);
  return {storage_name, StorageType_SPI, 0, total_bytes, free_bytes};
}

const char *esp32::s3::mini::v0::Name() {
  return "1.28\" 0.0";
}
static const esp_partition_t* int_ext_flash_hw(int mosi, int miso, int sclk, int cs){
  esp_err_t err;
  const spi_bus_config_t bus_config =
      {.mosi_io_num = mosi, .miso_io_num = miso, .sclk_io_num = sclk, .quadwp_io_num = -1, .quadhd_io_num = -1, .data4_io_num = -1, .data5_io_num = -1, .data6_io_num = -1, .data7_io_num = -1, .data_io_default_level = false, .max_transfer_sz = SPI_MAX_DMA_LEN, .flags = 0, .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO, .intr_flags = 0,};

  const esp_flash_spi_device_config_t
      device_config = {.host_id = SPI3_HOST, .cs_io_num = cs, .io_mode = SPI_FLASH_DIO, .input_delay_ns = 0, .cs_id = 0, .freq_mhz = 40};

  LOGI(TAG, "Initializing external SPI Flash");
  LOGI(TAG, "Pin assignments:");
  LOGI(TAG,
           "MOSI: %2d   MISO: %2d   SCLK: %2d   CS: %2d",
           bus_config.mosi_io_num,
           bus_config.miso_io_num,
           bus_config.sclk_io_num,
           device_config.cs_io_num);
  // Initialize the SPI bus
  LOGI(TAG, "DMA CHANNEL: %d", SPI_DMA_CH_AUTO);
  ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &bus_config, SPI_DMA_CH_AUTO));

  // Add device to the SPI bus
  esp_flash_t *ext_flash;
  ESP_ERROR_CHECK(spi_bus_add_flash_device(&ext_flash, &device_config));

  // Probe the Flash chip and initialize it
  err = esp_flash_init(ext_flash);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize external Flash: %s (0x%x)", esp_err_to_name(err), err);
  }

  // Print out the ID and size
  uint32_t id;
  uint32_t flash_size;
  ESP_ERROR_CHECK(esp_flash_read_id(ext_flash, &id));
  ESP_ERROR_CHECK(esp_flash_get_size(ext_flash, &flash_size));
  LOGI(TAG, "Initialized external Flash, size=%" PRIu32 " KB, ID=0x%" PRIx32, flash_size / 1024, id);
  LOGI(TAG,
           "Adding external Flash as a partition, label=\"%s\", size=%" PRIu32 " KB",
           "ext_data",
           flash_size / 1024);
  const esp_partition_t *fat_partition;
  ESP_ERROR_CHECK(esp_partition_register_external(ext_flash,
                                                  0,
                                                  flash_size,
                                                  "ext_data",
                                                  ESP_PARTITION_TYPE_DATA,
                                                  ESP_PARTITION_SUBTYPE_DATA_FAT,
                                                  &fat_partition));
  return fat_partition;
}

void esp32::s3::mini::v0::LateInit() {
  static wl_handle_t wl_handle = WL_INVALID_HANDLE;

  const esp_partition_t *fat_partition = int_ext_flash_hw(39, 41, 40, 42);
  ESP_ERROR_CHECK(wl_mount(fat_partition, &wl_handle));

#if !CONFIG_TINYUSB_MSC_ENABLED
  const esp_vfs_fat_mount_config_t mount_config = {
      .format_if_mount_failed = true,
      .max_files = 4,
      .allocation_unit_size = CONFIG_WL_SECTOR_SIZE,
  };
  ESP_ERROR_CHECK(esp_vfs_fat_spiflash_mount_rw_wl("/data", "ext_data", &mount_config, &wl_handle));
#else
  const tinyusb_msc_storage_config_t
      config_spi = {
        .medium = {wl_handle}, .fat_fs = {
          .base_path = storage_base_path,
          .config = {
            .format_if_mount_failed = false, .max_files = 5, .allocation_unit_size = 16 * 1024,
            .disk_status_check_enable = true, .use_one_fat = false
          },
          .do_not_format = true, .format_flags = 0
        },
        .mount_point = TINYUSB_MSC_STORAGE_MOUNT_USB
      };

  ESP_ERROR_CHECK(tinyusb_msc_new_storage_spiflash(&config_spi, &storage_handle));
#endif
  esp32s3_usb_init(GPIO_NUM_NC);
#if CFG_TUD_CDC
  tinyusb_console_init(TINYUSB_CDC_ACM_0);
#endif

  esp_pm_config_t pm_config = {.max_freq_mhz = 240, .min_freq_mhz = 240, .light_sleep_enable = false};
  esp_pm_configure(&pm_config);
}

bool esp32::s3::mini::v0::UsbConnected() {
#if CONFIG_TINYUSB_MSC_ENABLED
  tinyusb_msc_mount_point_t mount_status;
  tinyusb_msc_get_storage_mount_point(storage_handle, &mount_status);
  return mount_status == TINYUSB_MSC_STORAGE_MOUNT_USB;
#else
  return false;
#endif
}
int esp32::s3::mini::v0::UsbCallBack(tusb_msc_callback_t callback) {
#if CONFIG_TINYUSB_MSC_ENABLED
  tinyusb_msc_set_storage_callback(callback, nullptr);
#endif
  return 0;
}

}