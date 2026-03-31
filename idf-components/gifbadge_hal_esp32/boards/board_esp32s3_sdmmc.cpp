/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include <cstring>
#include <esp_check.h>
#include <esp_vfs_fat.h>
#include <driver/sdmmc_defs.h>
#include <esp_task_wdt.h>
#include <sdmmc_cmd.h>
#include <tinyusb.h>

#include "boards/esp32s3_sdmmc.h"
#include "log.h"
#include "tinyusb_msc.h"

static const char *TAG = "board_esp32s3_sdmmc";

namespace Boards {

StorageInfo esp32::s3::esp32s3_sdmmc::GetStorageInfo() {
  StorageType type = (card->ocr & SD_OCR_SDHC_CAP) ? StorageType_SDHC : StorageType_SD;
  double speed = card->real_freq_khz / 1000.00;
  uint64_t total_bytes;
  uint64_t free_bytes;
  esp_vfs_fat_info("/data", &total_bytes, &free_bytes);
  return {card->cid.name, type, speed, total_bytes, free_bytes};
}

void esp32::s3::esp32s3_sdmmc::Reset() {
#if CONFIG_TINYUSB_MSC_ENABLED
  if (storage_handle != nullptr) {
    if(tinyusb_msc_delete_storage(storage_handle) != ESP_OK){
      LOGI(TAG, "Failed to unmount");
    }
  }
#endif
  esp32s3::Reset();
}

void esp32::s3::esp32s3_sdmmc::PowerOff() {
#if CONFIG_TINYUSB_MSC_ENABLED
  if (storage_handle != nullptr) {
    if(tinyusb_msc_delete_storage(storage_handle) != ESP_OK){
      LOGI(TAG, "Failed to unmount");
    }
  }
#endif
}

int esp32::s3::esp32s3_sdmmc::StorageFormat() {
  LOGI(TAG, "Format Start");
  esp_err_t ret;
  esp_task_wdt_config_t wdtConfig = {
      .timeout_ms = 30 * 1000,
      .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,    // Bitmask of all cores
      .trigger_panic = false,
  };
#if CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU0
  wdtConfig.idle_core_mask |= (1 << 0);
#endif
#if CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU1
  wdtConfig.idle_core_mask |= (1 << 1);
#endif
  esp_task_wdt_reconfigure(&wdtConfig);
  ret = esp_vfs_fat_sdcard_format("/data", card);
  wdtConfig.timeout_ms = CONFIG_ESP_TASK_WDT_TIMEOUT_S * 1000;
  esp_task_wdt_reconfigure(&wdtConfig);
  LOGI(TAG, "Format Done");
  return ret;
}

const char *esp32::s3::esp32s3_sdmmc::GetStoragePath() {
  return "/data";
}

esp_err_t init_sdmmc_slot(sdmmc_host_t *host,
                          const sdmmc_slot_config_t *slot_config,
                          sdmmc_card_t **card) {
  esp_err_t ret = ESP_OK;
  bool host_init = false;

  LOGI(TAG, "Initializing SDCard");


  // not using ff_memalloc here, as allocation in internal RAM is preferred
  *card = static_cast<sdmmc_card_t *>(malloc(sizeof(sdmmc_card_t)));
  ESP_GOTO_ON_FALSE(*card, ESP_ERR_NO_MEM, clean, TAG, "could not allocate new sdmmc_card_t");

  ESP_GOTO_ON_ERROR((host->init)(), clean, TAG, "Host Config Init fail");
  host_init = true;

  ESP_GOTO_ON_ERROR(sdmmc_host_init_slot(host->slot, slot_config),
                    clean,
                    TAG,
                    "Host init slot fail");

  while (sdmmc_card_init(host, *card)) {
    ESP_LOGE(TAG, "The detection pin of the slot is disconnected(Insert uSD card). Retrying...");
    vTaskDelay(pdMS_TO_TICKS(3000));
  }

  // Card has been initialized, print its properties
  sdmmc_card_print_info(stdout, *card);

  return ESP_OK;

  clean:
  if (host_init) {
    if (host->flags & SDMMC_HOST_FLAG_DEINIT_ARG) {
      host->deinit_p(host->slot);
    } else {
      (*host->deinit)();
    }
  }
  if (*card) {
    free(*card);
    *card = nullptr;
  }
  return ret;
}

esp_err_t mount_sdmmc_slot(const sdmmc_host_t *host,
                           const sdmmc_slot_config_t *slot_config,
                           sdmmc_card_t **card) {
  esp_err_t ret = ESP_OK;

  LOGI(TAG, "Initializing SDCard");

  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
    .format_if_mount_failed = false, .max_files = 5, .allocation_unit_size = 16 * 1024,
    .disk_status_check_enable = true, .use_one_fat = false
  };
  ret = esp_vfs_fat_sdmmc_mount("/data", host, slot_config, &mount_config, card);
  sdmmc_card_print_info(stdout, *card);
  return ret;

}


esp_err_t esp32::s3::esp32s3_sdmmc::mount(const gpio_num_t clk,
                                          const gpio_num_t cmd,
                                          const gpio_num_t d0,
                                          const gpio_num_t d1,
                                          const gpio_num_t d2,
                                          const gpio_num_t d3,
                                          const gpio_num_t cd,
                                          const int width,
                                          gpio_num_t usb_sense) {
  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;

  const sdmmc_slot_config_t slot_config = {
    .clk = clk,
    .cmd = cmd,
    .d0 = d0,
    .d1 = d1,
    .d2 = d2,
    .d3 = d3,
    .d4 = GPIO_NUM_NC,
    .d5 = GPIO_NUM_NC,
    .d6 = GPIO_NUM_NC,
    .d7 = GPIO_NUM_NC,
    .cd = cd,
    .wp = SDMMC_SLOT_NO_WP,
    .width   = static_cast<uint8_t>(width),
    .flags = 0,
};


#if CONFIG_TINYUSB_MSC_ENABLED
  if (init_sdmmc_slot(&host, &slot_config, &card) == ESP_OK) {
    constexpr tinyusb_msc_driver_config_t usb_config = {
      .user_flags = {.auto_mount_off = 0},
      .callback = nullptr,
      .callback_arg = nullptr
    };

    ESP_ERROR_CHECK(tinyusb_msc_install_driver(&usb_config));

    const tinyusb_msc_storage_config_t config_sdmmc =
    {
      .medium = {.card = card},
      .fat_fs = {
        .base_path = storage_base_path,
        .config = {
          .format_if_mount_failed = false, .max_files = 5, .allocation_unit_size = 16 * 1024,
          .disk_status_check_enable = true, .use_one_fat = false
        },
        .do_not_format = true, .format_flags = 0
      },
      .mount_point = TINYUSB_MSC_STORAGE_MOUNT_APP
    };
    ESP_ERROR_CHECK(tinyusb_msc_new_storage_sdmmc(&config_sdmmc, &storage_handle));

    ESP_ERROR_CHECK(esp32s3_usb_init(usb_sense));

    storageAvailable = true;
    char str[12];
    /* Get volume label of the default drive */
    f_getlabel("", str, nullptr);
    LOGI(TAG, "Volume Name: %s", str);
    if (strlen(str) == 0) {
      f_setlabel("GifBadge");
    }
    return ESP_OK;
  }
  return ESP_FAIL;
#else
  ESP_ERROR_CHECK(esp32s3_usb_init(usb_sense));
  return mount_sdmmc_slot(&host, &slot_config, &card);
#endif
}

bool esp32::s3::esp32s3_sdmmc::UsbConnected() {
  if(!storageAvailable){
    return false;
  }
#if CONFIG_TINYUSB_MSC_ENABLED
  tinyusb_msc_mount_point_t mount_status;
  tinyusb_msc_get_storage_mount_point(storage_handle, &mount_status);
  return mount_status == TINYUSB_MSC_STORAGE_MOUNT_USB;
#else
  return false;
#endif
}
int esp32::s3::esp32s3_sdmmc::UsbCallBack(tusb_msc_callback_t callback) {
#if CONFIG_TINYUSB_MSC_ENABLED
  tinyusb_msc_set_storage_callback(callback, nullptr);
#endif
  return 0;
}
}