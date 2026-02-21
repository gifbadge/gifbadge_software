/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include "filebuffer.h"

#include <cstring>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_heap_caps.h>
#else
#include "FreeRTOS.h"
#include "task.h"
#include <cstdlib>
#endif

#define MAX_FILE_LEN 128

#define BUFFER_CHUNK (256)
#define BUFFER_KEEP_SIZE (BUFFER_CHUNK*8)

static TaskHandle_t file_buffer_task = nullptr;
static TaskHandle_t wakeup_task = nullptr;

#define displayNotifyIndex 1

#define DEBUG 0

#ifdef DEBUG
#include <cerrno>
#include <cstdio>
#endif

#define debug_print(...) \
do { if (DEBUG) printf(__VA_ARGS__); } while (0)

static struct circular_buf_t {
  uint8_t *data = nullptr;
  uint8_t * write_pos = nullptr;
  uint8_t * read_pos = nullptr;
  int32_t file_pos = 0;
  size_t file_size = 0;
  int fd = 0;
  char open_file[MAX_FILE_LEN+1] = "";
  bool start_valid = false;
  uint8_t * start_pos = nullptr;
  size_t size = 0;
} cbuffer;

static size_t cbuffer_get_free(const circular_buf_t *buffer) {
  if (buffer->write_pos == buffer->read_pos) {
    return buffer->size;
  }
  if (buffer->read_pos >= buffer->write_pos) {
    return buffer->read_pos - buffer->write_pos;
  }
  return (buffer->read_pos + buffer->size + 1) - buffer->write_pos;
}

static size_t cbuffer_get_avail(const circular_buf_t *buffer) {
  return buffer->size - cbuffer_get_free(buffer);
}

static void cbuffer_reset(circular_buf_t *buffer) {
  buffer->write_pos = buffer->data;
  buffer->read_pos = buffer->data;
  buffer->file_pos = 0;
}

static void cbuffer_init(circular_buf_t *buffer, size_t size) {
#ifdef ESP_PLATFORM
  buffer->data = static_cast<uint8_t *>(heap_caps_malloc(size+1, MALLOC_CAP_SPIRAM));
#else
  buffer->data = static_cast<uint8_t *>(malloc(size+1));
#endif
  memset(buffer->data, 0, size);

  buffer->size  = size;
  cbuffer_reset(buffer);
}

static size_t cbuffer_put_file(circular_buf_t *buffer, size_t size) {
  size_t count = 0;

  if (buffer->write_pos >= buffer->read_pos) {
    size_t end_free_segment = (buffer->data + buffer->size) - buffer->write_pos + (buffer->read_pos == buffer->data ? 0 : 1);

    if (end_free_segment >= size)
    {
      count = read(buffer->fd, buffer->write_pos, size);
      buffer->write_pos += size;
    } else {
      count = read(buffer->fd, buffer->write_pos, end_free_segment);
      count += read(buffer->fd, buffer->data, size - end_free_segment);
      buffer->write_pos = buffer->data + size - end_free_segment;
    }
  } else {
    count = read(buffer->fd, buffer->write_pos, size);
    buffer->write_pos += size;
  }

  return count;
}

static void wakeup_sleeping_task() {
  if (wakeup_task != nullptr) {
      xTaskNotifyIndexed(wakeup_task, displayNotifyIndex, 0, eNoAction);
  } else {
    debug_print("wakeup_task == nullptr\n");
  }
}

static UBaseType_t wait_for_task(TickType_t delay) {
  wakeup_task = xTaskGetCurrentTaskHandle();
  return xTaskNotifyWaitIndexed(displayNotifyIndex, 0, 0xffffffff, nullptr, delay);
}

static int32_t cbuffer_read(circular_buf_t *buffer, uint8_t *dest, int32_t size) {
  if (buffer->write_pos >= buffer->read_pos) {
    memcpy(dest, buffer->read_pos, size);
  } else {
    size_t end_data_segment = buffer->data + buffer->size - buffer->read_pos + (buffer->read_pos == buffer->data ? 0 : 1);
    if (end_data_segment >= size)
    {
      memcpy(dest, buffer->read_pos, size);
    } else {
      memcpy(dest, buffer->read_pos, end_data_segment);
      memcpy(dest + end_data_segment, buffer->data, size - end_data_segment);
    }
  }
  buffer->read_pos += size;
  if (buffer->read_pos > buffer->data + buffer->size)
    buffer->read_pos -= buffer->size + 1;
  buffer->file_pos += size;
  return size;
}

static void cbuffer_close(circular_buf_t *buffer) {
  if (buffer->fd > 0) {
    close(buffer->fd);
    buffer->fd = 0;
    debug_print("File Closed\n");
  }
}

static void cbuffer_open(circular_buf_t *buffer, const char *path) {
  cbuffer_close(buffer);
  cbuffer_reset(&cbuffer);
  if (strlen(path) > 0) {
    strncpy(cbuffer.open_file, path, MAX_FILE_LEN);
    buffer->fd = open(path, O_RDONLY);
    if (buffer->fd == -1) {
      debug_print("Couldn't open file %s: %s\n", path, strerror(errno));
    } else {
      struct stat buf{};
      fstat(buffer->fd, &buf);
      buffer->file_size = buf.st_size;
      debug_print("Opened %s size %lu\n", path, buffer->file_size);
    }
  }
}

void FileBufferTask(void *params) {
  debug_print("Starting FileBuffer task\n");
  file_buffer_task = xTaskGetCurrentTaskHandle();
  cbuffer_init(&cbuffer, *static_cast<size_t *>(params));

  TickType_t delay = portMAX_DELAY;

  while (true) {
    uint32_t notify_option = 0;
    int ret = xTaskNotifyWaitIndexed(0, 0, 0xffffffff, &notify_option, delay);
    // debug_print("%d, %d\n", ret, notify_option);
    switch (notify_option) {
      case FILEBUFFER_CLOSE:
        cbuffer_close(&cbuffer);
        wakeup_sleeping_task();
        delay = portMAX_DELAY;
        continue;
      case FILEBUFFER_PAUSE:
        wakeup_sleeping_task();
        debug_print("FILEBUFFER_PAUSE\n");
        delay = portMAX_DELAY;
        continue;
      case FILEBUFFER_STOP:
        debug_print("FILEBUFFER_STOP\n");
        cbuffer_close(&cbuffer);
        free(cbuffer.data);
        vTaskDelete(nullptr);
        return;
      default:
        break;
    }
    delay = 0;
    if (cbuffer.fd > 0) {
      if ((cbuffer_get_free(&cbuffer) - (BUFFER_KEEP_SIZE)) >= BUFFER_CHUNK) {
        size_t count = cbuffer_put_file(&cbuffer, BUFFER_CHUNK);
        if (count < BUFFER_CHUNK) {
          if (cbuffer.file_size > cbuffer.size) {
            cbuffer.start_pos = cbuffer.write_pos;
            lseek(cbuffer.fd, 0, SEEK_SET);
            cbuffer_put_file(&cbuffer, BUFFER_CHUNK);
            cbuffer.start_valid = true;
          } else {
            delay = portMAX_DELAY;
          }
        }
      } else {
        delay = portMAX_DELAY;
      }
      wakeup_sleeping_task();
    } else {
      delay = portMAX_DELAY;
    }
  }
}

bool filebuffer_open(const char *path) {
  xTaskNotifyIndexed(file_buffer_task, 0, FILEBUFFER_PAUSE, eSetValueWithOverwrite);
  if (wait_for_task(10/portTICK_PERIOD_MS) != pdTRUE) {
    debug_print("Timeout during file open\n");
    return false;
  }
  cbuffer_open(&cbuffer, path);
  xTaskNotifyIndexed(file_buffer_task, 0, 0, eNoAction);
  return true;
}

void filebuffer_close() {
  xTaskNotifyIndexed(file_buffer_task, 0, FILEBUFFER_CLOSE, eSetValueWithOverwrite);
  if (wait_for_task(10/portTICK_PERIOD_MS) != pdTRUE) {
    debug_print("Timeout during file close\n");
  }
}

int32_t filebuffer_read(uint8_t *pBuf, const int32_t iLen) {
  size_t available;
  while (available = cbuffer_get_avail(&cbuffer), available < iLen) {
  xTaskNotifyIndexed(file_buffer_task, 0, 0, eNoAction);
    if (wait_for_task(10/portTICK_PERIOD_MS) != pdTRUE) {
      debug_print("Timeout while reading file\n");
      return 0;
    }
  }
  int32_t ret =  cbuffer_read(&cbuffer, pBuf, iLen);
  xTaskNotifyIndexed(file_buffer_task, 0, 0, eNoAction);
  return ret;
}

void filebuffer_seek(int32_t pos) {
  xTaskNotifyIndexed(file_buffer_task, 0, FILEBUFFER_PAUSE, eSetValueWithOverwrite);
  if (wait_for_task(10/portTICK_PERIOD_MS) != pdTRUE) {
    debug_print("Timeout during file seek\n");
    return;
  }
  int32_t new_pos = pos - cbuffer.file_pos;
  if ((cbuffer.file_size > cbuffer.size) && new_pos < -BUFFER_KEEP_SIZE) {
    if (cbuffer.start_valid) {
      cbuffer.read_pos = cbuffer.start_pos;
    } else {
      cbuffer_open(&cbuffer, cbuffer.open_file);
    }
    cbuffer.start_valid = false;
    cbuffer.file_pos = 0;
  }
  else {
    cbuffer.read_pos += new_pos;
    cbuffer.file_pos += new_pos;
  }
  xTaskNotifyIndexed(file_buffer_task, 0, 0, eNoAction);
}
