/*******************************************************************************
 * Copyright (c) 2025-2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/
#include "FreeRTOS.h"
#include "task.h"
#include "input.h"
#include "display.h"
#include "ui/menu.h"
#include "hw_init.h"

extern MAIN_STATES currentState;

static void powerOff() {
  get_board()->PowerOff();
}

static void imageCurrent() {
  if (currentState == MAIN_NORMAL) {
    TaskHandle_t display_task_handle = xTaskGetHandle("display_task");
    xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_FILE, eSetValueWithOverwrite);
  }
}

static void imageNext() {
  if (currentState == MAIN_NORMAL) {
    TaskHandle_t display_task_handle = xTaskGetHandle("display_task");
    xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_NEXT, eSetValueWithOverwrite);
  }
}

static void imagePrevious() {
  if (currentState == MAIN_NORMAL) {
    TaskHandle_t display_task_handle = xTaskGetHandle("display_task");
    xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_PREVIOUS, eSetValueWithOverwrite);
  }
}

static void imageSpecial1() {
  if (currentState == MAIN_NORMAL) {
    TaskHandle_t display_task_handle = xTaskGetHandle("display_task");
    xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_SPECIAL_1, eSetValueWithOverwrite);
  }
}

static void imageSpecial2() {
  if (currentState == MAIN_NORMAL) {
    TaskHandle_t display_task_handle = xTaskGetHandle("display_task");
    xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_SPECIAL_2, eSetValueWithOverwrite);
  }
}

static void openMenu();

static const keyCommands keyOptions[hal::keys::KEY_MAX] = {
    keyCommands{imageNext, imageSpecial1, 300*1000},
    keyCommands{imagePrevious, imageSpecial2, 300*1000},
    keyCommands{openMenu, powerOff, 5000*1000}
};

hal::keys::EVENT_STATE inputState;
static int lastKey;
static long long lastKeyPress;

#ifdef ESP_PLATFORM
#include <esp_timer.h>
static esp_timer_handle_t inputTimer = nullptr;

static void openMenu() {
  if (currentState == MAIN_NORMAL) {
    esp_timer_stop(inputTimer);
    lvgl_menu_open();
  }
}

void startInputTimer() {
  if (!esp_timer_is_active(inputTimer)) {
    ESP_ERROR_CHECK(esp_timer_start_periodic(inputTimer, 10 * 1000));
  }
}

static void inputTimerHandler(void *args) {
  auto board = (Boards::Board *) args;
  board->GetKeys()->poll();
  hal::keys::EVENT_STATE *key_state = board->GetKeys()->read();

  switch (inputState) {
    case hal::keys::STATE_RELEASED:
      for (int b = 0; b < hal::keys::KEY_MAX; b++) {
        if (key_state[b] == hal::keys::STATE_PRESSED) {
          lastKey = b;
          inputState = hal::keys::STATE_PRESSED;
          lastKeyPress = esp_timer_get_time();
        }
      }
      break;
    case hal::keys::STATE_PRESSED:
      if (key_state[lastKey] == hal::keys::STATE_RELEASED) {
        keyOptions[lastKey].press();
        inputState = hal::keys::STATE_RELEASED;
      } else if (esp_timer_get_time() - lastKeyPress > keyOptions[lastKey].delay) {
        if (key_state[lastKey] == hal::keys::STATE_HELD) {
          keyOptions[lastKey].hold();
          inputState = hal::keys::STATE_HELD;
        }
      }
      break;
    case hal::keys::STATE_HELD:
      if (key_state[lastKey] == hal::keys::STATE_RELEASED) {
        imageCurrent();
        inputState = hal::keys::STATE_RELEASED;
      }
      break;
  }
}

void initInputTimer(Boards::Board *board) {
  const esp_timer_create_args_t inputTimerArgs = {
      .callback = &inputTimerHandler,
      .arg = board,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "input_handler",
      .skip_unhandled_events = true
  };

  ESP_ERROR_CHECK(esp_timer_create(&inputTimerArgs, &inputTimer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(inputTimer, 1 * 1000));
}
#else
#include "portable_time.h"
#include "timers.h"

TimerHandle_t inputTimer;

static void openMenu() {
  if (currentState == MAIN_NORMAL) {
    xTimerStop(inputTimer, portMAX_DELAY);
    lvgl_menu_open();
  }
}

void startInputTimer() {
  xTimerStart(inputTimer,0);
}

static void inputTimerHandler(TimerHandle_t) {
  auto board = get_board();
  if (currentState == MAIN_NORMAL) {
      hal::keys::EVENT_STATE *key_state = board->GetKeys()->read();

      switch (inputState) {
        case hal::keys::STATE_RELEASED:
          for (int b = 0; b < hal::keys::KEY_MAX; b++) {
            if (key_state[b] == hal::keys::STATE_PRESSED) {
              lastKey = b;
              inputState = hal::keys::STATE_PRESSED;
              lastKeyPress = millis();
            }
          }
          break;
        case hal::keys::STATE_PRESSED:
          if (key_state[lastKey] == hal::keys::STATE_RELEASED) {
            keyOptions[lastKey].press();
            inputState = hal::keys::STATE_RELEASED;
          } else if (millis() - lastKeyPress > 300 * 1000) {
            if (key_state[lastKey] == hal::keys::STATE_HELD) {
              keyOptions[lastKey].hold();
              inputState = hal::keys::STATE_HELD;
            }
          }
          break;
        case hal::keys::STATE_HELD:
          if (key_state[lastKey] == hal::keys::STATE_RELEASED) {
            imageCurrent();
            inputState = hal::keys::STATE_RELEASED;
          }
          break;
      }
//TODO: Fix touch
//      if (board->getTouch()) {
//        auto e = board->getTouch()->read();
//        if (e.first > 0 && e.second > 0) {
//          if (((esp_timer_get_time() / 1000) - (last_change / 1000)) > 1000) {
//            last_change = esp_timer_get_time();
//            if (e.second < 50) {
//              openMenu();
//            }
//            if (e.first < 50) {
//              imageNext();
//            }
//            if (e.first > 430) {
//              imagePrevious();
//            }
//          }
//          LOGI(TAG, "x: %d y: %d", e.first, e.second);
//        }
//      }
  }
}

void initInputTimer(Boards::Board *board) {
  inputTimer = xTimerCreate("input", 5/portTICK_PERIOD_MS, pdTRUE, nullptr, inputTimerHandler);
  xTimerStart(inputTimer,0);
}
#endif