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

#include "hal/battery.h"
#include "ui/main_menu.h"
#include "ui/style.h"
#include "hw_init.h"
#include "display.h"
#include "ui/widgets/battery/lv_battery.h"
#include "timers.h"
#include "touch.h"

static const char *TAG = "MENU";

//Static Variables
static lv_disp_t *disp;
static TaskHandle_t lvgl_task;
static flushCbData cbData;
static SemaphoreHandle_t flushSem;
static SemaphoreHandle_t lvgl_open = nullptr;

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

void lvgl_close() {
  LOGI(TAG, "Close");
  get_board()->GetDisplay()->onColorTransDone(nullptr);
  startInputTimer();

  destroy_screens();
  lv_obj_clean(lv_layer_top());

  vTaskDelay(100 / portTICK_PERIOD_MS); //Wait some time so the task can finish
  get_board()->PmRelease();
  LOGI(TAG, "Close Done");
}

void battery_percent_update(lv_obj_t *widget) {
  auto battery = static_cast<hal::battery::Battery *>(lv_obj_get_user_data(widget));
  if (battery->BatteryStatus() == hal::battery::Battery::State::ERROR || battery->BatteryStatus() == hal::battery::Battery::State::NOT_PRESENT) {
    lv_battery_set_value(widget, 0);
  } else {
    lv_battery_set_value(widget, battery->BatterySoc());
  }
}

void battery_symbol_update(lv_obj_t *cont) {
  auto battery = static_cast<hal::battery::Battery *>(lv_obj_get_user_data(cont));
  lv_obj_t *symbol = lv_obj_get_child(cont, 0);
  lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_remove_flag(cont, LV_OBJ_FLAG_HIDDEN);
  if (battery->BatteryStatus() == hal::battery::Battery::State::ERROR) {
    lv_image_set_src(symbol, LV_SYMBOL_DUMMY "\uf22f");
    lv_obj_set_style_text_color(symbol, lv_color_hex(0xeed202), LV_PART_MAIN); //Yellow
  } else if (battery->BatteryStatus() == hal::battery::Battery::State::NOT_PRESENT) {
    lv_image_set_src(symbol, LV_SYMBOL_DUMMY "\ue5cd");
    lv_obj_set_style_text_color(symbol, lv_color_hex(0xff0033), LV_PART_MAIN); //Red
  } else if (battery->BatteryStatus() == hal::battery::Battery::State::CHARGING) {
    lv_image_set_src(symbol, LV_SYMBOL_DUMMY "\uea0b");
    lv_obj_set_style_text_color(symbol, lv_color_hex(0x50C878), LV_PART_MAIN); //Green
  } else {
    lv_image_set_src(symbol, LV_SYMBOL_DUMMY);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, LV_PART_MAIN);
  }
}



static void battery_widget(lv_obj_t *scr) {
  lv_obj_t *battery_bar = lv_obj_create(scr);
  lv_obj_set_size(battery_bar, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_add_style(battery_bar, &style_battery_bar, LV_PART_MAIN);

  lv_obj_t *bar = lv_battery_create(battery_bar);
  lv_obj_add_style(bar, &style_battery_indicator, LV_PART_INDICATOR);
  lv_obj_add_style(bar, &style_battery_main, LV_PART_MAIN);
  lv_obj_set_size(bar, 20, 40);
  lv_obj_center(bar);
  lv_obj_set_user_data(bar, get_board()->GetBattery());


  lv_obj_add_event_cb(bar, [](lv_event_t *e) {
    battery_percent_update(static_cast<lv_obj_t *>(lv_event_get_target(e)));
  }, LV_EVENT_REFRESH, nullptr);
  battery_percent_update(bar);

  lv_timer_t *timer = lv_timer_create([](lv_timer_t *timer) {
    auto *obj = static_cast<lv_obj_t *>(timer->user_data);
    lv_obj_send_event(obj, LV_EVENT_REFRESH, nullptr);
  }, 30000, bar);
  lv_obj_add_event_cb(bar, [](lv_event_t *e) {
    auto *timer = static_cast<lv_timer_t *>(e->user_data);
    lv_timer_del(timer);
  }, LV_EVENT_DELETE, timer);

  lv_obj_t *battery_symbol_cont = lv_obj_create(battery_bar);
  lv_obj_remove_style_all(battery_symbol_cont);
  lv_obj_add_style(battery_symbol_cont, &style_battery_icon_container, LV_PART_MAIN);
  lv_obj_set_size(battery_symbol_cont, 24, 24);
  lv_obj_align_to(battery_symbol_cont, bar, LV_ALIGN_BOTTOM_RIGHT, 18, 12);

  lv_obj_t *battery_symbol = lv_img_create(battery_symbol_cont);
  lv_obj_add_style(battery_symbol, &style_battery_icon, LV_PART_MAIN);
  lv_obj_align(battery_symbol, LV_ALIGN_CENTER, 0, 0);

  lv_obj_set_user_data(battery_symbol_cont, get_board()->GetBattery());

  lv_obj_add_event_cb(battery_symbol_cont, [](lv_event_t *e) {
    battery_symbol_update(static_cast<lv_obj_t *>(lv_event_get_target(e)));
  }, LV_EVENT_REFRESH, nullptr);
  battery_symbol_update(battery_symbol_cont);

  lv_timer_t *timer_icon = lv_timer_create([](lv_timer_t *timer) {
    auto *obj = static_cast<lv_obj_t *>(timer->user_data);
    lv_obj_send_event(obj, LV_EVENT_REFRESH, nullptr);
  }, 30000, battery_symbol_cont);
  lv_obj_add_event_cb(battery_symbol_cont, [](lv_event_t *e) {
    auto *timer = static_cast<lv_timer_t *>(e->user_data);
    lv_timer_del(timer);
  }, LV_EVENT_DELETE, timer_icon);

}

void battery_draw() {
  lv_group_t *g = lv_group_create();
  lv_group_set_default(g);
  lv_indev_set_group(lvgl_encoder, g);
  lv_obj_t *scr = create_screen();
  lv_screen_load(scr);
  battery_widget(lv_layer_top());
}

void task(void *) {
  bool running = true;
  LOGI(TAG, "Starting LVGL task");
  int32_t task_delay = -1;
  TaskHandle_t display_task_handle = nullptr;

  while (running) {
    uint32_t option;
    xTaskNotifyWaitIndexed(0, 0, 0xffffffff, &option, task_delay == -1 ? portMAX_DELAY : task_delay/portTICK_PERIOD_MS);
    switch (option) {
      case LVGL_STOP:
        LOGI(TAG, "LVGL_STOP");
        lvgl_close();
        xSemaphoreGive(lvgl_open);
        if (cbData.display) {
          vTaskDelay(200 / portTICK_PERIOD_MS);
          cbData.display->clear();
          cbData.display->write(0, 0, cbData.display->size.first, cbData.display->size.second, cbData.display->buffer);
        }
        display_task_handle = xTaskGetHandle("display_task");
        xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_NONE, eSetValueWithOverwrite); //Notify the display task to redraw
        task_delay = -1; //Block until woken up
        break;
      case LVGL_EXIT:
        running = false;
        break;
      case LVGL_RESUME_MENU:
        task_delay = 0;
        xSemaphoreTake(lvgl_open, 100);
        battery_draw();
        main_menu();
        LOGI(TAG, "LVGL_RESUME");
        break;
      case LVGL_RESUME_USB:
        task_delay = 0;
        xSemaphoreTake(lvgl_open, 100);
        battery_draw();
        lvgl_usb_connected();
        LOGI(TAG, "LVGL_RESUME");
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

bool lvgl_menu_state() {
  if (xSemaphoreTake(lvgl_open, 0)) {
    xSemaphoreGive(lvgl_open);
    return false;
  }
  return true;
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
  lvgl_open = xSemaphoreCreateMutex();

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



void lvgl_wake_up() {
  if (xSemaphoreTake(lvgl_open, 100)) {
    xSemaphoreGive(lvgl_open);
    LOGI(TAG, "Wakeup");
    get_board()->PmLock();

    cbData.display = get_board()->GetDisplay();

    cbData.callbackEnabled = cbData.display->onColorTransDone(flush_ready);

    xSemaphoreGive(flushSem);
    LOGI(TAG, "Wakeup Done");
  }
}

void lvgl_menu_open() {
  lvgl_wake_up();
  xTaskNotifyIndexed(lvgl_task, 0, LVGL_RESUME_MENU, eSetValueWithOverwrite);
}

void lvgl_usb_open() {
  lvgl_wake_up();
  xTaskNotifyIndexed(lvgl_task, 0, LVGL_RESUME_USB, eSetValueWithOverwrite);
}



