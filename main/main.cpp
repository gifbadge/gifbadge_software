/*******************************************************************************
 * Copyright (c) 2025-2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include "log.h"
#include "ui/menu.h"
#include "display.h"

#include "hw_init.h"
#include "input.h"
#include "filebuffer.h"

static const char *TAG = "MAIN";

#ifdef CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS
TaskStatus_t tasks[20];

const char *taskState(eTaskState i) {
  switch (i) {
    case eRunning:
      return "eRunning";
    case eReady:
      return "eReady";
    case eBlocked:
      return "eBlocked";
    case eSuspended:
      return "eSuspended";
    case eDeleted:
      return "eDeleted";
    case eInvalid:
      return "eInvalid";
    default:
      return "Unknown";
  }
}

#endif

void dumpDebugFunc(TimerHandle_t) {
  auto *args = get_board();
  args->PmLock();
  args->DebugInfo();
#ifdef CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS
  unsigned long ulTotalRunTime;
  unsigned int count = uxTaskGetSystemState(tasks, 20, &ulTotalRunTime);
  // ulTotalRunTime /= 100UL;
  float runtime = ulTotalRunTime / 100.0f;
  for (unsigned int i = 0; i < count; i++) {
    LOGI(TAG,

      "%s State: %s, Highwater: %lu, Runtime: %0.2f",
      tasks[i].pcTaskName,
      taskState(tasks[i].eCurrentState),
      tasks[i].usStackHighWaterMark,
      tasks[i].ulRunTimeCounter > 0?static_cast<float>(tasks[i].ulRunTimeCounter)/runtime:0.0f
      );
  }
#endif
  args->PmRelease();

}

static void dumpDebugTimerInit() {
  xTimerStart(xTimerCreate("dumpDebugTimer", 60000/portTICK_PERIOD_MS, pdTRUE, nullptr, dumpDebugFunc), 0);
}

/*!
 * Task to check battery status, and update device
 * @param args
 */
static void lowBatteryTask(TimerHandle_t) {
  static bool lastState = false;
  if (get_board()->PowerState() == Boards::BOARD_POWER_CRITICAL) {
    if (!lastState) {
      lvgl_low_battery_open();
      lastState = true;
    } else {
      get_board()->PowerOff();
    }
  }
  else {
    if (lastState) {
      lastState = false;
      if (!get_board()->UsbConnected()) {
        lvgl_menu_close();
      }
    }
  }
}

static void initLowBatteryTask() {
  xTimerStart(xTimerCreate("low_battery_handler", 10000/portTICK_PERIOD_MS, pdTRUE, nullptr, lowBatteryTask),0);
}

#if defined(ESP_PLATFORM)
#include <sdkconfig.h>
#if CONFIG_TINYUSB_MSC_ENABLED
static void usbCall(tinyusb_msc_storage_handle_t handle, tinyusb_msc_event_t *e, void *arg){
  LOGI(TAG, "USB Event %d", e->id);
  if (e->id != TINYUSB_MSC_EVENT_MOUNT_COMPLETE) {
    return;
  }
  TaskHandle_t lvglHandle = xTaskGetHandle("LVGL");

  auto board = get_board();
  LOGI(TAG, "USB Callback %s", e->mount_point == TINYUSB_MSC_STORAGE_MOUNT_USB?"USB Mounted":"App mounted");
  if(e->mount_point == TINYUSB_MSC_STORAGE_MOUNT_USB){
    lvgl_usb_open();
  } else if(e->mount_point == TINYUSB_MSC_STORAGE_MOUNT_APP){
    xTaskNotifyIndexed(lvglHandle, 0, LVGL_STOP, eSetValueWithOverwrite);
    //Check for OTA File
    if (board->OtaCheck()) {
      // vTaskDelay(1000 / portTICK_PERIOD_MS);
      lvgl_ota_open();
      board->OtaInstall();
    }
  }
}
#endif
#endif



extern "C" void app_main(void) {
  Boards::Board *board = get_board();
  switch(board->BootReason()){

    case Boards::WakeupSource::NONE:
      LOGI(TAG, "Wakeup Reason: None");
      break;
    case Boards::WakeupSource::VBUS:
      LOGI(TAG, "Wakeup Reason: VBUS");
      break;
    case Boards::WakeupSource::KEY:
      LOGI(TAG, "Wakeup Reason: KEY");
      break;
    case Boards::WakeupSource::REBOOT:
      LOGI(TAG, "Wakeup Reason: REBOOT");
      break;
  }
  if(!(board->BootReason() == Boards::WakeupSource::KEY || board->BootReason() == Boards::WakeupSource::REBOOT)){
    board->PowerOff();
  }

  board->LateInit();

  TaskHandle_t display_task_handle = nullptr;
  TaskHandle_t file_buffer_task = nullptr;

  dumpDebugTimerInit();
  board->BootInfo();

  lvgl_init(board);

  initLowBatteryTask();

  size_t buffer_size = 0;
  if (board->MemorySize() > 4*1024*1024) {
    buffer_size = 4*1024*1024;
  }
  else {
    buffer_size = 1*1024*1024;
  }


#ifdef ESP_PLATFORM
  xTaskCreatePinnedToCore(display_task, "display_task", 6000, board, 2, &display_task_handle, 1);
  xTaskCreatePinnedToCore(FileBufferTask, "file_buffer", 4000, &buffer_size, 2, &file_buffer_task, 0);
#else
  xTaskCreate(display_task, "display_task", 5000, board, 2, &display_task_handle);
  xTaskCreate(FileBufferTask, "file_buffer", 4000, &buffer_size, 2, &file_buffer_task);
#endif

  initInputTimer(board);
  
  if (!board->StorageReady()) {
    lvgl_no_sd_open();
    LOGI(TAG, "Storage not Ready");
    return;
  }

#if defined(ESP_PLATFORM) && defined(CONFIG_TINYUSB_MSC_ENABLED)
  board->UsbCallBack(&usbCall);
#endif

  vTaskDelay(500 / portTICK_PERIOD_MS);

  if(!board->UsbConnected()){
    xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_FILE, eSetValueWithOverwrite);
  }

#ifndef ESP_PLATFORM
  while (true) {
    vTaskDelay(1000/portTICK_RATE_MS);
  }
#endif

}

#ifdef ESP_PLATFORM
IRAM_ATTR void vApplicationTickHook() {
  lv_tick(nullptr);
}
#else
void vApplicationTickHook() {
  lv_tick(nullptr);
}
#endif


#ifndef ESP_PLATFORM
#include <thread>
#include "drivers/display_sdl.h"
#include "drivers/key_sdl.h"
#include "drivers/touch_sdl.h"
#include <unistd.h>
#include <sys/stat.h>
#include <csignal>

// extern "C" void vLoggingPrintf(const char *pcFormat, ...) {
//   va_list arg;
//
//   va_start( arg, pcFormat );
//   vprintf( pcFormat, arg );
//   va_end( arg );
// }

void vApplicationIdleHook( void )
{
  /* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
   * to 1 in FreeRTOSConfig.h.  It will be called on each iteration of the idle
   * task.  It is essential that code added to this hook function never attempts
   * to block in any way (for example, call xQueueReceive() with a block time
   * specified, or call vTaskDelay()).  If application tasks make use of the
   * vTaskDelete() API function to delete themselves then it is also important
   * that vApplicationIdleHook() is permitted to return to its calling function,
   * because it is the responsibility of the idle task to clean up memory
   * allocated by the kernel to any task that has since deleted itself. */


  // usleep( 15000 );
}

void vAssertCalled( const char * const pcFileName,
                    unsigned long ulLine )
{
  static BaseType_t xPrinted = pdFALSE;
  volatile uint32_t ulSetToNonZeroInDebuggerToContinue = 0;

  /* Called if an assertion passed to configASSERT() fails.  See
   * https://www.FreeRTOS.org/a00110.html#configASSERT for more information. */

  /* Parameters are not used. */
  ( void ) ulLine;
  ( void ) pcFileName;


  taskENTER_CRITICAL();
  {
    /* Stop the trace recording. */
    if( xPrinted == pdFALSE )
    {
      xPrinted = pdTRUE;

#if ( projENABLE_TRACING == 1 )
      {
                prvSaveTraceFile();
            }
#endif /* if ( projENABLE_TRACING == 0 ) */
    }

    /* You can step out of this function to debug the assertion by using
     * the debugger to set ulSetToNonZeroInDebuggerToContinue to a non-zero
     * value. */
    while( ulSetToNonZeroInDebuggerToContinue == 0 )
    {
      __asm volatile ( "NOP" );
      __asm volatile ( "NOP" );
    }
  }
  taskEXIT_CRITICAL();
}

void vApplicationMallocFailedHook( void )
{
  /* vApplicationMallocFailedHook() will only be called if
   * configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
   * function that will get called if a call to pvPortMalloc() fails.
   * pvPortMalloc() is called internally by the kernel whenever a task, queue,
   * timer or semaphore is created.  It is also called by various parts of the
   * demo application.  If heap_1.c, heap_2.c or heap_4.c is being used, then the
   * size of the    heap available to pvPortMalloc() is defined by
   * configTOTAL_HEAP_SIZE in FreeRTOSConfig.h, and the xPortGetFreeHeapSize()
   * API function can be used to query the size of free heap space that remains
   * (although it does not provide information on how the remaining heap might be
   * fragmented).  See http://www.freertos.org/a00111.html for more
   * information. */
  vAssertCalled( __FILE__, __LINE__ );
}

void vApplicationStackOverflowHook( TaskHandle_t pxTask,
                                    char * pcTaskName )
{
  ( void ) pcTaskName;
  ( void ) pxTask;

  /* Run time stack overflow checking is performed if
   * configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
   * function is called if a stack overflow is detected.  This function is
   * provided as an example only as stack overflow checking does not function
   * when running the FreeRTOS POSIX port. */
  vAssertCalled( __FILE__, __LINE__ );
}

bool running = true;

void die(void *) {
  xTaskNotifyIndexed(xTaskGetHandle("display_task"), 0, DISPLAY_STOP, eSetValueWithOverwrite);
  xTaskNotifyIndexed(xTaskGetHandle("file_buffer"), 0, FILEBUFFER_STOP, eSetValueWithOverwrite);
  vTaskDelay(1000/portTICK_PERIOD_MS);
  vTaskEndScheduler();
}

void handle_sigint(int) {
  running = false;
  xTaskCreate(die, "die", 10000, nullptr, 1, nullptr);
}

bool usbState = false;

static void usbCall(){
  TaskHandle_t lvglHandle = xTaskGetHandle("LVGL");
  usbState = !usbState;
  if(usbState){
    lvgl_usb_open();
  } else{
    xTaskNotifyIndexed(lvglHandle, 0, LVGL_STOP, eSetValueWithOverwrite);
  }
}

extern "C" int main(void) {
  //chroot the process, so it's closer to being on the device for paths, etc
  // char path[255];
  // getcwd(path, sizeof(path));
  // printf("Chrooting to %s\n", path);
  // unshare(CLONE_FS|CLONE_NEWUSER);
  // chroot(path);
  // chdir("/");
  // mkdir("/data", 0700);

  console_init();
  console_print("test\n");
  signal( SIGINT, handle_sigint );
  signal( SIGTERM, handle_sigint );
  auto display = display_sdl_init({480, 480});
  auto keys = keys_sdl_init();
  auto touch = touch_sdl_init();
  xTaskCreate(reinterpret_cast<void (*)(void *)>(app_main), "app_main", 10000, nullptr, 1, nullptr);
  std::thread schedular([](){
    vTaskStartScheduler();
  });
  // sleep(10);
  // handle_sigint(0);
  while (running) {
    display->update();
    SDL_Event event;
    /* Poll for events. SDL_PollEvent() returns 0 when there are no  */
    /* more events on the event queue, our while loop will exit when */
    /* that occurs.                                                  */
    while (SDL_PollEvent(&event)) {
      /* We are only worried about SDL_KEYDOWN and SDL_KEYUP events */
      if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
        if ( event.key.scancode == SDL_SCANCODE_U && event.type == SDL_EVENT_KEY_UP) {
          usbCall();
        } else {
          keys->update(event);
        }
      }
      else if (event.type ==SDL_EVENT_MOUSE_MOTION ||event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ||event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
        touch->update(event);
      }
    }
    usleep(10*1000);
  }
  schedular.join();
  return 0;
}

#endif