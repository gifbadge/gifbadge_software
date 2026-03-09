/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include "FreeRTOS.h"
#include "task.h"
#include <semphr.h>
#include "log.h"

#include <lvgl.h>

#include "ui/menu.h"

#include <cassert>
#include <input.h>
#include <ui/usb_connected.h>

#include "ui/main_menu.h"
#include "ui/style.h"
#include "hw_init.h"
#include "display.h"
#include "timers.h"
#include "touch.h"
#include "ui/battery.h"
#include "ui/no_sd.h"
#include "ui/ota.h"

static const char *TAG = "MENU";

//Static Variables
static lv_disp_t *disp;
static TaskHandle_t lvgl_task;
static flushCbData cbData;
static SemaphoreHandle_t flushSem;

//Exported Variables
extern "C" {
lv_indev_t *lvgl_encoder;
lv_indev_t *lvgl_touch;
}

std::vector<lv_obj_t *> screens;

lv_obj_t * create_screen(){
  lv_obj_t *scr = lv_obj_create(nullptr);
  screens.emplace_back(scr);
  return scr;
}

void destroy_screens(){
  for(auto &scr:screens){
    if(lv_obj_is_valid(scr)) {
      lv_obj_delete(scr);
    }
  }
  screens.clear();
}

static bool flush_ready() {
  xSemaphoreGive(flushSem);
  return false;
}

static void flush_cb(lv_disp_t *, const lv_area_t *area, uint8_t *color_map) {
  assert(disp != nullptr);
  cbData.display->write(
      area->x1,
      area->y1,
      area->x2 + 1,
      area->y2 + 1,
      color_map);
  lv_display_set_buffers(disp, cbData.display->buffer, nullptr, cbData.display->size.first * cbData.display->size.second * 2,
                         LV_DISPLAY_RENDER_MODE_FULL);
}

void lv_tick(TimerHandle_t) {
  lv_tick_inc(portTICK_PERIOD_MS);
}

bool lvgl_status = false;

void lvgl_close() {
  LOGI(TAG, "Close");
  lvgl_status = false;
  get_board()->GetDisplay()->onColorTransDone(nullptr);
  startInputTimer();

  destroy_screens();
  lv_obj_clean(lv_layer_top());

  get_board()->PmRelease();
  LOGI(TAG, "Close Done");
}

static void lvgl_wake_up() {
    LOGI(TAG, "Wakeup");
    lvgl_status = true;
    get_board()->PmLock();
    stopInputTimer();

    cbData.display = get_board()->GetDisplay();

    cbData.callbackEnabled = cbData.display->onColorTransDone(flush_ready);

    xSemaphoreGive(flushSem);
    LOGI(TAG, "Wakeup Done");
}

static void lvgl_screen_init() {
  lv_group_t *g = lv_group_create();
  lv_group_set_default(g);
  lv_indev_set_group(lvgl_encoder, g);
  lv_obj_t *scr = create_screen();
  lv_screen_load(scr);
}

void task(void *) {
  bool running = true;
  LOGI(TAG, "Starting LVGL task");
  int32_t task_delay = -1;
  TaskHandle_t display_task_handle = nullptr;

  while (running) {
    uint32_t option;
    xTaskNotifyWaitIndexed(0, 0, 0xffffffff, &option, task_delay == -1 ? portMAX_DELAY : task_delay);
    switch (option) {
      case LVGL_STOP:
        LOGI(TAG, "LVGL_STOP");
        lvgl_close();
        display_task_handle = xTaskGetHandle("display_task");
        xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_FILE, eSetValueWithOverwrite); //Notify the display task to redraw
        task_delay = -1; //Block until woken up
        LOGI(TAG, "LVGL_STOP done");
        break;
      case LVGL_EXIT:
        running = false;
        break;
      case LVGL_RESUME_MENU:
        lvgl_wake_up();
        task_delay = 0;
        display_task_handle = xTaskGetHandle("display_task");
        xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_MENU, eSetValueWithOverwrite);
        xTaskNotifyWaitIndexed(0, 0, 0xffffffff, &option, portMAX_DELAY);
        lvgl_screen_init();
        battery_widget(lv_layer_top());
        main_menu();
        LOGI(TAG, "LVGL_RESUME");
        break;
      case LVGL_RESUME_USB:
        lvgl_wake_up();
        task_delay = 0;
        display_task_handle = xTaskGetHandle("display_task");
        xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_NOTIFY_USB, eSetValueWithOverwrite);
        xTaskNotifyWaitIndexed(0, 0, 0xffffffff, &option, portMAX_DELAY);
        lvgl_screen_init();
        battery_widget(lv_layer_top());
        lvgl_usb_connected();
        LOGI(TAG, "LVGL_RESUME_USB");
        break;
      case LVGL_RESUME_OTA:
        if (lvgl_status == false) {
          lvgl_wake_up();
          task_delay = 0;
          display_task_handle = xTaskGetHandle("display_task");
          xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_NOTIFY_USB, eSetValueWithOverwrite);
          xTaskNotifyWaitIndexed(0, 0, 0xffffffff, &option, portMAX_DELAY);
          lvgl_screen_init();
          battery_widget(lv_layer_top());
        }
        lvgl_ota();
        LOGI(TAG, "LVGL_RESUME_OTA");
        break;
      case LVGL_RESUME_NOSD:
        lvgl_wake_up();
        task_delay = 0;
        display_task_handle = xTaskGetHandle("display_task");
        xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_MENU, eSetValueWithOverwrite);
        xTaskNotifyWaitIndexed(0, 0, 0xffffffff, &option, portMAX_DELAY);
        lvgl_screen_init();
        battery_widget(lv_layer_top());
        lvgl_nosd();
        LOGI(TAG, "LVGL_RESUME_NOSD");
        break;
      default:
        task_delay = static_cast<int32_t>(lv_timer_handler());
        if (task_delay > LVGL_TASK_MAX_DELAY_MS) {
          task_delay = LVGL_TASK_MAX_DELAY_MS;
        } else if (task_delay < LVGL_TASK_MIN_DELAY_MS) {
          task_delay = LVGL_TASK_MIN_DELAY_MS;
        }
        break;
    }
  }
}

void keyboard_read(lv_indev_t *indev, lv_indev_data_t *data) {
  auto g = lv_indev_get_group(indev);
  bool editing = lv_group_get_editing(g);
  auto *device = static_cast<hal::keys::Keys *>(lv_indev_get_user_data(indev));
  if(device) {
    device->poll();
    hal::keys::EVENT_STATE *keys = device->read();
    if (keys[hal::keys::KEY_UP] == hal::keys::STATE_PRESSED) {
      data->enc_diff += editing ? +1 : -1;
    } else if (keys[hal::keys::KEY_DOWN] == hal::keys::STATE_PRESSED) {
      data->enc_diff += editing ? -1 : +1;
    } else if (keys[hal::keys::KEY_ENTER] == hal::keys::STATE_PRESSED || keys[hal::keys::KEY_ENTER] == hal::keys::STATE_HELD) {
      data->state = LV_INDEV_STATE_PRESSED;
    } else {
      data->state = LV_INDEV_STATE_RELEASED;
    }
  }

}

void touch_read(lv_indev_t *drv, lv_indev_data_t *data) {

  auto touch = static_cast<hal::touch::Touch *>(lv_indev_get_driver_data(drv));
  auto i = touch->read();
  if (i.first > 0 && i.second > 0) {
    data->point.x = static_cast<int32_t>(i.first);
    data->point.y = static_cast<int32_t>(i.second);
    data->state = LV_INDEV_STATE_PRESSED;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

static void flushWaitCb(lv_display_t *){
  xSemaphoreTake(flushSem, portMAX_DELAY);
}

void lvgl_init(Boards::Board *board) {
  lv_init();

  flushSem = xSemaphoreCreateBinary();

#ifdef ESP_PLATFORM
  xTaskCreate(task, "LVGL", 7*1024, nullptr, LVGL_TASK_PRIORITY, &lvgl_task);
#else
  xTaskCreate(task, "LVGL", 7*1024, nullptr, LVGL_TASK_PRIORITY, &lvgl_task);
#endif

  disp = lv_display_create(board->GetDisplay()->size.first, board->GetDisplay()->size.second);
  lv_display_set_flush_cb(disp, flush_cb);
  lv_display_set_flush_wait_cb(disp, flushWaitCb);
  lv_display_set_buffers(disp, board->GetDisplay()->buffer, nullptr,
                         board->GetDisplay()->size.first * board->GetDisplay()->size.second * 2,
                         LV_DISPLAY_RENDER_MODE_FULL);

  style_init();
  lvgl_encoder = lv_indev_create();
  lv_indev_set_type(lvgl_encoder, LV_INDEV_TYPE_ENCODER);
  lv_indev_set_user_data(lvgl_encoder, board->GetKeys());
  lv_indev_set_read_cb(lvgl_encoder, keyboard_read);
  lv_timer_set_period(lv_indev_get_read_timer(lvgl_encoder), 2);

  if (board->GetTouch()) {
    lvgl_touch = lv_indev_create();
    lv_indev_set_type(lvgl_touch, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(lvgl_touch, touch_read);
    lv_indev_set_driver_data(lvgl_touch, board->GetTouch());
    lv_timer_set_period(lv_indev_get_read_timer(lvgl_touch), 10);
  }

}

void lvgl_menu_open() {
  xTaskNotifyIndexed(lvgl_task, 0, LVGL_RESUME_MENU, eSetValueWithOverwrite);
}

void lvgl_usb_open() {
  xTaskNotifyIndexed(lvgl_task, 0, LVGL_RESUME_USB, eSetValueWithOverwrite);
}

void lvgl_ota_open() {
  xTaskNotifyIndexed(lvgl_task, 0, LVGL_RESUME_OTA, eSetValueWithOverwrite);
}

void lvgl_no_sd_open() {
  xTaskNotifyIndexed(lvgl_task, 0, LVGL_RESUME_NOSD, eSetValueWithOverwrite);
}



