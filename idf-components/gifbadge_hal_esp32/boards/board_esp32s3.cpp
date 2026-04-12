/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include <esp_app_desc.h>
#include <esp_pm.h>
#include <esp_mac.h>
#include <esp_ota_ops.h>
#include <tinyusb.h>
#include <soc/gpio_sig_map.h>
#include <sys/stat.h>
#include "boards/boards_esp32s3.h"
#include "log.h"
#include "esp_efuse_custom_table.h"
#include "esp_app_format.h"
#include "esp_ota.h"
#include "esp_psram.h"
#include "../../../main/include/hw_init.h"
#include "usb_descriptors.h"
#if CFG_TUD_CDC
#include <tinyusb_cdc_acm.h>
#endif

static const char *TAG = "esp32::s3::esp32s3";

namespace Boards::esp32::s3 {
esp32s3::esp32s3() {
  esp_pm_lock_create(ESP_PM_CPU_FREQ_MAX, 0, "Board Lock", &pmLockHandle);
  _config = new hal::config::esp32s3::Config_NVS(this);
}
void esp32s3::Reset() {
  esp_restart();
}

void esp32s3::PmLock() {
  esp_pm_lock_acquire(pmLockHandle);
}

void esp32s3::PmRelease() {
  esp_pm_lock_release(pmLockHandle);
}

hal::config::Config *esp32s3::GetConfig() {
  return _config;
}

void esp32s3::DebugInfo() {
  ESP_LOGI(TAG, "Free Heap: %db", esp_get_free_heap_size());
  ESP_LOGI(TAG, "Free PSRAM: %db/%db", heap_caps_get_free_size(MALLOC_CAP_SPIRAM), heap_caps_get_total_size(MALLOC_CAP_SPIRAM));
  ESP_LOGI(TAG, "Free Internal: %db/%db", heap_caps_get_free_size(MALLOC_CAP_INTERNAL), heap_caps_get_total_size(MALLOC_CAP_INTERNAL));
  ESP_LOGI(TAG, "Largest Free Block for DMA: %db", heap_caps_get_largest_free_block(MALLOC_CAP_DMA));
  ESP_LOGI(TAG, "USB State: %d", UsbConnected());
}

const char *esp32s3::SwVersion() {
  return esp_app_get_description()->version;
}
char *esp32s3::SerialNumber() {
  uint8_t mac[6] = {0};
  esp_efuse_mac_get_default(mac);
  sprintf(serial, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return serial;
}
void esp32s3::BootInfo() {
  const esp_partition_t *configured = esp_ota_get_boot_partition();
  const esp_partition_t *running = esp_ota_get_running_partition();

  LOGI(TAG, "Total Application Partitions: %d", esp_ota_get_app_partition_count());
  LOGI(TAG,
       "Running partition type %d subtype %d (offset 0x%08" PRIx32 ")",
       running->type,
       running->subtype,
       running->address);

  esp_app_desc_t boot_app_info;
  if (esp_ota_get_partition_description(configured, &boot_app_info) == ESP_OK) {
    LOGI(TAG, "Configured firmware version: %s", boot_app_info.version);
  }
  esp_app_desc_t running_app_info;
  if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
    LOGI(TAG, "Running firmware version: %s", running_app_info.version);
  }
}
bool esp32s3::OtaCheck() {
  struct stat buffer{};
  if (stat("/data/ota.bin", &buffer) == 0) {
    LOGI("TAG", "OTA File Exists");
    return true;
  }
  return false;
}

#define OTA_HEADER_SIZE (sizeof(esp_image_header_t)+ sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t) + sizeof(esp_custom_app_desc_t))

OtaError esp32s3::OtaHeaderValidate(uint8_t const *data) {
  assert(data != nullptr);
  OtaError ret = OtaError::OK;
  auto *new_header_info = static_cast<esp_image_header_t *>(malloc(sizeof(esp_image_header_t)));
  auto *new_app_info = static_cast<esp_app_desc_t *>(malloc(sizeof(esp_app_desc_t)));
  auto *new_custom_app_desc = static_cast<esp_custom_app_desc_t *>(malloc(sizeof(esp_custom_app_desc_t)));
  size_t pos = 0;
  memcpy(new_header_info, data, sizeof(esp_image_header_t));
  pos += sizeof(esp_image_header_t);
  pos += sizeof(esp_image_segment_header_t);
  memcpy(new_app_info, data + pos, sizeof(esp_app_desc_t));
  pos += sizeof(esp_app_desc_t);
  memcpy(new_custom_app_desc, data + pos, sizeof(esp_custom_app_desc_t));
  pos += sizeof(esp_custom_app_desc_t);

  LOGI(TAG, "Header Size %u", pos);
  LOGI(TAG, "New Firmware");
  LOGI(TAG, "CHIPID: %i", new_header_info->chip_id);
  LOGI(TAG, "Version: %s", new_app_info->version);
  LOGI(TAG, "Supports Boards:");
  for (int x = 0; x < new_custom_app_desc->num_supported_boards; x++) {
    LOGI(TAG, "%u", new_custom_app_desc->supported_boards[x]);
  }

  if (new_header_info->chip_id != ESP_CHIP_ID_ESP32S3) {
    ret = OtaError::WRONG_CHIP;
  } else {
    uint8_t board;
    esp_efuse_read_field_blob(ESP_EFUSE_USER_DATA_BOARD, &board, 8);
    bool supported_board = false;
    for (int x = 0; x < new_custom_app_desc->num_supported_boards; x++) {
      if (board == new_custom_app_desc->supported_boards[x]) {
        supported_board = true;
        break;
      }
    }
    if (!supported_board) {
      ret = OtaError::WRONG_BOARD;
    }
  }

  free(new_header_info);
  free(new_app_info);
  free(new_custom_app_desc);
  return ret;
}

OtaError esp32s3::OtaValidate() {
  OtaError ret = OtaError::OK;
  auto *ota_data = static_cast<uint8_t *>(malloc(OTA_HEADER_SIZE));
  FILE *ota_file = fopen("/data/ota.bin", "r");
  fread(ota_data, OTA_HEADER_SIZE, 1, ota_file);
  fclose(ota_file);

  if (OtaError valid = OtaHeaderValidate(ota_data); valid != OtaError::OK) {
    ret = valid;
  }

  free(ota_data);
  return ret;
}

void esp32s3::OtaInstall() {
  if (!_ota_task_handle) {
    //Free the turbo buffer, if present, so we are sure to have enough stack for the update task
    if (TurboBuffer()) {
      free(TurboBuffer());
    }
    ESP_LOGI(TAG, "Free Internal: %d", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    xTaskCreate(OtaInstallTask, "ota", 4000, this, 2, &_ota_task_handle);
  }
}
int esp32s3::OtaStatus() {
  return _ota_status;
}

OtaError esp32s3::OtaState() {
  return _ota_error;
}

void esp32s3::_ota_error_set(const char *str) {
  strncpy(_ota_error_str, str, 63);
}

const char * esp32s3::OtaString() {
  return _ota_error_str;
}

void esp32s3::OtaInstallTask(void *arg) {
  esp_err_t err;
  auto *board = static_cast<esp32s3 *>(arg);

  size_t ota_size = 0;
  struct stat buffer{};
  if (stat("/data/ota.bin", &buffer) == 0) {
    ota_size = buffer.st_size;
  }
  LOGI(TAG, "OTA Update is %zu", ota_size);

  esp_ota_handle_t update_handle = 0;
  const esp_partition_t *update_partition;

  const esp_partition_t *configured = esp_ota_get_boot_partition();
  const esp_partition_t *running = esp_ota_get_running_partition();

  if (configured != running) {
    ESP_LOGW(TAG,
             "Configured OTA boot partition at offset 0x%08" PRIx32", but running from offset 0x%08" PRIx32,
             configured->address,
             running->address);
    ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
  }
  LOGI(TAG,
       "Running partition type %d subtype %d (offset 0x%08" PRIx32")",
       running->type,
       running->subtype,
       running->address);

  update_partition = esp_ota_get_next_update_partition(nullptr);
  assert(update_partition != nullptr);
  LOGI(TAG,
       "Writing to partition subtype %d at offset 0x%" PRIx32,
       update_partition->subtype,
       update_partition->address);

  if (OtaError validation_err = board->OtaValidate(); validation_err != OtaError::OK) {
    ESP_LOGE(TAG, "validate failed with %d", (int) validation_err);
    board->_ota_error = validation_err;
#ifdef CONFIG_ESP_ERR_TO_NAME_LOOKUP
    switch (validation_err) {
      case OtaError::WRONG_CHIP:
        board->_ota_error_set("Wrong Chip");
        break;
      case OtaError::WRONG_BOARD:
        board->_ota_error_set("Wrong Board");
        break;
      default:
        break;
    }
#endif
    board->_ota_error = OtaError::FAIL;
    vTaskDelete(nullptr);
  }

  err = esp_ota_begin(update_partition, ota_size, &update_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
#ifdef CONFIG_ESP_ERR_TO_NAME_LOOKUP
    board->_ota_error_set(esp_err_to_name(err));
#endif
    board->_ota_error = OtaError::FAIL;
    esp_ota_abort(update_handle);
    vTaskDelete(nullptr);
  }

#define OTA_BUFFER_SIZE (4096+1)

  FILE *ota_file = fopen("/data/ota.bin", "r");
  static char *ota_buffer = static_cast<char *>(malloc(OTA_BUFFER_SIZE));
  if (ota_buffer == nullptr) {
    board->_ota_status = -2;
#ifdef CONFIG_ESP_ERR_TO_NAME_LOOKUP
    board->_ota_error_set("Not enough memory");
#endif
    board->_ota_error = OtaError::FAIL;
  }

  size_t bytes_read;

  board->_ota_status = 0;
  while ((bytes_read = fread(ota_buffer, 1, OTA_BUFFER_SIZE, ota_file)) > 0) {
    //Update the progress on the display
    int percent = static_cast<int>((100 * ftell(ota_file) + ota_size / 2) / ota_size);
    LOGI(TAG, "%%%d", percent);
    board->_ota_status = percent;

    err = esp_ota_write(update_handle, ota_buffer, bytes_read);
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "esp_ota_write failed (%s)!", esp_err_to_name(err));
#ifdef CONFIG_ESP_ERR_TO_NAME_LOOKUP
      board->_ota_error_set(esp_err_to_name(err));
#endif
      board->_ota_error = OtaError::FAIL;
      esp_ota_abort(update_handle);
    }
  }

  LOGI(TAG, "Writing Done. Deleting OTA File");
  fclose(ota_file);
  remove("/data/ota.bin");

  err = esp_ota_end(update_handle);
  if (err != ESP_OK) {
      ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
#ifdef CONFIG_ESP_ERR_TO_NAME_LOOKUP
    board->_ota_error_set(esp_err_to_name(err));
#endif
    board->_ota_error = OtaError::FAIL;
    if (ota_buffer) {
      free(ota_buffer);
    }
    vTaskDelete(nullptr);
  }

  err = esp_ota_set_boot_partition(update_partition);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
#ifdef CONFIG_ESP_ERR_TO_NAME_LOOKUP
    board->_ota_error_set(esp_err_to_name(err));
#endif
    board->_ota_error = OtaError::FAIL;
    if (ota_buffer) {
      free(ota_buffer);
    }
    vTaskDelete(nullptr);
  }
  LOGI(TAG, "Prepare to restart system!");
  board->Reset();
}
size_t esp32s3::MemorySize() {
  return esp_psram_get_size();
}
esp_err_t esp32s3::esp32s3_usb_init(const gpio_num_t usb_sense) {
#if CFG_TUD_CDC

  constexpr tinyusb_config_cdcacm_t acm_cfg = {
    .cdc_port = TINYUSB_CDC_ACM_0,
    .callback_rx = nullptr,
    .callback_rx_wanted_char = nullptr,
    .callback_line_state_changed = nullptr,
    .callback_line_coding_changed = nullptr
};
  tinyusb_cdcacm_init(&acm_cfg);
#endif

  int str_count = 0;

  const char *descriptor_str[] = {
    // array of pointer to string descriptors
    (char[]){0x09, 0x04},                // 0: is supported language is English (0x0409)
    "GifBadge", // 1: Manufacturer
    Name(),      // 2: Product
    SerialNumber(),       // 3: Serials

  #if CONFIG_TINYUSB_CDC_ENABLED
    CONFIG_TINYUSB_DESC_CDC_STRING,          // 4: CDC Interface
  #endif

  #if CONFIG_TINYUSB_MSC_ENABLED
    "GifBadge Storage",          // 5: MSC Interface
  #endif
    "FLASH",
    nullptr                                     // NULL: Must be last. Indicates end of array
  };

  while (descriptor_str[++str_count] != nullptr){}

  const tinyusb_config_t tusb_cfg =
  {
    .port = TINYUSB_PORT_FULL_SPEED_0,
    .phy = {.skip_setup = false, .self_powered = true, .vbus_monitor_io = usb_sense},
    .task = {.size = 4096, .priority = 5, .xCoreID = 0}, .descriptor = {
      .device = &descriptor_dev,
      .qualifier = nullptr,
      .string = descriptor_str,
      .string_count = str_count,
      .full_speed_config = descriptor_fs_cfg,
      .high_speed_config = nullptr,
    }, .event_cb = {}, .event_arg = nullptr
  };
  return tinyusb_driver_install(&tusb_cfg);
}

void esp32s3::VbusHandler(const bool state) {
  if (state) {
    esp_rom_gpio_connect_in_signal(GPIO_MATRIX_CONST_ONE_INPUT, USB_SRP_BVALID_IN_IDX, false);
  } else {
    esp_rom_gpio_connect_in_signal(GPIO_MATRIX_CONST_ZERO_INPUT, USB_SRP_BVALID_IN_IDX, false);
  }
}

}

#ifdef CONFIG_TINYUSB_DFU_MODE_DFU
#include <class/dfu/dfu_device.h>

//--------------------------------------------------------------------+
// DFU callbacks
// Note: alt is used as the partition number, in order to support multiple partitions like FLASH, EEPROM, etc.
//--------------------------------------------------------------------+

// Invoked right before tud_dfu_download_cb() (state=DFU_DNBUSY) or tud_dfu_manifest_cb() (state=DFU_MANIFEST)
// Application return timeout in milliseconds (bwPollTimeout) for the next download/manifest operation.
// During this period, USB host won't try to communicate with us.
uint32_t tud_dfu_get_timeout_cb(uint8_t, uint8_t state) {
  if (state == DFU_DNBUSY) {
    return 1;
  } else if (state == DFU_MANIFEST) {
    // since we don't buffer entire image and do any flashing in manifest stage
    return 0;
  }

  return 0;
}

esp_ota_handle_t update_handle = 0;

// Invoked when received DFU_DNLOAD (wLength>0) following by DFU_GETSTATUS (state=DFU_DNBUSY) requests
// This callback could be returned before flashing op is complete (async).
// Once finished flashing, application must call tud_dfu_finish_flashing()
void tud_dfu_download_cb(uint8_t, uint16_t block_num, uint8_t const *data, uint16_t length) {
  esp_err_t err;

  LOGI(TAG, "Download BlockNum %u of length %u", block_num, length);
  if (block_num == 0 && length >= OTA_HEADER_SIZE) {
    const esp_partition_t *update_partition;

    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    if (configured != running) {
      ESP_LOGW(TAG,
               "Configured OTA boot partition at offset 0x%08" PRIx32", but running from offset 0x%08" PRIx32,
               configured->address,
               running->address);
      ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }
    LOGI(TAG,
         "Running partition type %d subtype %d (offset 0x%08" PRIx32")",
         running->type,
         running->subtype,
         running->address);

    update_partition = esp_ota_get_next_update_partition(nullptr);
    assert(update_partition != nullptr);
    LOGI(TAG,
         "Writing to partition subtype %d at offset 0x%" PRIx32,
         update_partition->subtype,
         update_partition->address);

    if (Boards::OtaError validation_err = Boards::esp32::s3::esp32s3::OtaHeaderValidate(data); validation_err != Boards::OtaError::OK) {
      ESP_LOGE(TAG, "validate failed with %d", (int) validation_err);
      tud_dfu_finish_flashing(DFU_STATUS_ERR_TARGET);
      return;
    }

    err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
      tud_dfu_finish_flashing(DFU_STATUS_ERR_WRITE);
      return;
    }
  }
  err = esp_ota_write(update_handle, data, length);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_ota_write failed (%s)!", esp_err_to_name(err));
    esp_ota_abort(update_handle);
    tud_dfu_finish_flashing(DFU_STATUS_ERR_WRITE);
    return;
  }

  tud_dfu_finish_flashing(DFU_STATUS_OK);
}

// Invoked when download process is complete, received DFU_DNLOAD (wLength=0) following by DFU_GETSTATUS (state=Manifest)
// Application can do checksum, or actual flashing if buffered entire image previously.
// Once finished flashing, application must call tud_dfu_finish_flashing()
void tud_dfu_manifest_cb(uint8_t) {
  esp_err_t err;
  ESP_LOGI(TAG, "Download completed, Verifying");

  err = esp_ota_end(update_handle);
  if (err != ESP_OK) {
    if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
      ESP_LOGE(TAG, "Image validation failed, image is corrupted");
      tud_dfu_finish_flashing(DFU_STATUS_ERR_VERIFY);
      return;
    }
    ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
    tud_dfu_finish_flashing(DFU_STATUS_ERR_VERIFY);
    return;
  }

  err = esp_ota_set_boot_partition(esp_ota_get_next_update_partition(nullptr));
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
    tud_dfu_finish_flashing(DFU_STATUS_ERR_VENDOR);
    return;
  }
  LOGI(TAG, "Verify Success");

  // flashing op for manifest is complete without error
  // Application can perform checksum, should it fail, use appropriate status such as errVERIFY.
  tud_dfu_finish_flashing(DFU_STATUS_OK);
}

// Invoked when received DFU_UPLOAD request
// Application must populate data with up to length bytes and
// Return the number of written bytes
uint16_t tud_dfu_upload_cb(uint8_t, uint16_t, uint8_t *, uint16_t) {
  return 0;
}

// Invoked when the Host has terminated a download or upload transfer
void tud_dfu_abort_cb(uint8_t) {
  LOGI(TAG, "Host aborted transfer");
}

// Invoked when a DFU_DETACH request is received
void tud_dfu_detach_cb(void) {
  LOGI(TAG, "Host detach, we should probably reboot");
  LOGI(TAG, "Prepare to restart system!");
  get_board()->Reset();
}

#endif
