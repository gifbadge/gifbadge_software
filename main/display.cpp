/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include "FreeRTOS.h"

#include "portable_time.h"
#include "log.h"

#include "display.h"
#include <memory>
#include <sys/stat.h>

#include "image.h"
#include "font_render.h"

#include "directory.h"
#include "file.h"
#include "dirname.h"
#include "hash_path.h"
#include "hw_init.h"
#include "resize.h"

static const char *TAG = "DISPLAY";

static DIR_SORTED dir;

static int file_position;

#define MAX_FILE_LEN 128

namespace image {

/**
 * Render a string as an image to display errors/etc
 */
class ErrorImage : public Image {
 public:
  ErrorImage(screenResolution size, const char *error)
      : Image(size), _error("") {
    if (error != nullptr) {
      strcpy(_error, error);
    }
  }

  template<typename ... Args>
  ErrorImage(screenResolution size, const char *fmt, Args &&... args)
      : Image(size), _error("") {
    snprintf(_error, sizeof(_error) - 1, fmt, std::forward<Args>(args) ...);
  };

  frameReturn GetFrame(uint8_t *outBuf, int16_t x, int16_t y) override {
    for (int i = 0; i < resolution.first * resolution.second; i++) {
      reinterpret_cast<uint16_t *>(outBuf)[i] = 0x2966;
    }
    render_text_centered(resolution.first, resolution.second, 10, _error, outBuf);
    return {frameStatus::OK, _delay};
  }
  std::pair<int16_t, int16_t> Size() override {
    return resolution;
  }
  const char *GetLastError() override {
    return nullptr;
  }
 protected:
  char _error[255];
  int _delay = 1000;
};

class NoImage : public ErrorImage {
 public:
  explicit NoImage(screenResolution size) : ErrorImage(size, nullptr) {
    strcpy(_error, "No Image");
  }
};

class TooLargeImage : public ErrorImage {
 public:
  TooLargeImage(screenResolution size, const char *path) : ErrorImage(size, nullptr) {
    sprintf(_error, "Image too Large\n%s", path);
  }
};

class ResizingImage: public ErrorImage {
  public:
  explicit ResizingImage(std::pair<int16_t, int16_t> size) : ErrorImage(size, nullptr) {
    strcpy(_error, "Resizing");
  }
};
}

static std::pair<int16_t, int16_t> lastSize = {0,0};

bool newImage = false;

int64_t average_frame_delay = 0;
int64_t average_frame_time = 0;
int max_frame_delay = 0;
int frame_count = 0;
float last_fps = 0;
bool looped = false;
int64_t image_start = 0;

static image::frameReturn displayFile(std::unique_ptr<image::Image> &in, hal::display::Display *display) {
  int64_t start = millis();
  image::frameReturn status;
  int16_t xOffset = 0;
  int16_t yOffset = 0;
  if (in->Size() != display->size) {
    if (newImage && (lastSize > in->Size())) {
      display->clear(); //Only need to clear the screen if the image won't fill it, and the last image was bigger
      newImage = false;
    }
    xOffset = static_cast<int16_t>((display->size.first / 2) - ((in->Size().first +1) / 2));
    yOffset = static_cast<int16_t>((display->size.second / 2) - ((in->Size().second + 1) / 2));
  }
  status = in->GetFrame(display->buffer, xOffset, yOffset);
  if (status.first == image::frameStatus::ERROR) {
    LOGI(TAG, "Image loop error. Frame %d", frame_count);
    return {image::frameStatus::ERROR, 0};
  }
  display->write(0, 0, display->size.first, display->size.second, display->buffer);
  int frameTime = static_cast<int>(millis() - start);
  int calc_delay = static_cast<int>(status.second) - frameTime;
  // LOGI(TAG, "Frame Delay: %lu, calculated delay %i", status.second, calc_delay);
  frame_count += 1;
  average_frame_delay += calc_delay;
  average_frame_time += frameTime + (calc_delay > 0 ? calc_delay : 0) ;
  if (calc_delay < max_frame_delay) {
    max_frame_delay = calc_delay;
  }
  lastSize = in->Size();
  if(in->Animated()) {
    if (status.first == image::frameStatus::END) {
      if (looped) {
        last_fps = 1000.00f/(static_cast<float>(average_frame_time)/static_cast<float>(frame_count));
        LOGI(TAG, "Average FPS: %f", last_fps);
        LOGI(TAG, "FPS: %f", 1000.00f/(static_cast<float>(millis()-image_start)/static_cast<float>(frame_count)));
      }
      LOGI(TAG, "Average Frame Delay: %lld, Max Delay: %li", average_frame_delay/frame_count, max_frame_delay);

      frame_count = 0;
      average_frame_time = 0;
      average_frame_delay = 0;
      max_frame_delay = 0;
      looped = true;
      image_start = millis();
    }
    return {status.first, (calc_delay > 0 ? calc_delay : 0)/portTICK_PERIOD_MS};
  }
  else{
    LOGI(TAG, "Frame Time: %lld", millis()-start, max_frame_delay);
    return {image::frameStatus::END, portMAX_DELAY};
  }
}

static int validator(const char *path, const char *file) {
  char inPath[128];
  JOIN_PATH(inPath, path, file);
  return valid_image_file(inPath, extensions);
}

static int get_file(char *path) {
  //Check if we are starting with a valid file, and just return it if we are
  if (valid_image_file(path, extensions)) {
    if(!dir.dirptr){
      char *base = basename(path);
      base[-1] = '\0'; //Replace the slash with a null, so we can pretend the string is shorter
      opendir_sorted(&dir, path, validator);
      base[-1] = '/'; //return the slash, fix the string
    }
    file_position = directory_get_position(&dir, basename(path));
    LOGI(TAG, "%i", file_position);
    return 0;
  }
  char *base = path;
  //Check if it's a directory
  if (!is_directory(path)) {
    //It's not a directory. Get the directory
    base = dirname(path);
  }
  struct dirent *de;  // Pointer for directory entry

  closedir_sorted(&dir);

  if (!opendir_sorted(&dir, base, validator)) {
    return -1;
  }

  while ((de = readdir_sorted(&dir)) != nullptr) {
    size_t len = strlen(path);
    if(path[len-1] != '/'){
      path[len] = '/';
      path[len+1] = '\0';
    }
    LOGI(TAG, "%s", path);
    strcpy(&path[strlen(path)], de->d_name);
    file_position = directory_get_position(&dir, de->d_name);
    LOGI(TAG, "%i", file_position);
    LOGI(TAG, "%s", path);
    return 0;
  }
//  path[0] = '\0';
  return -1;
}

static void get_cache(const char *path, char *cache_path) {
  strcpy(cache_path, get_board()->GetStoragePath());
  strcat(cache_path, "/.cache/");
  uint8_t result[16];
  hash_path(path, result);
  for(const uint8_t i : result){
    char v[3];
    sprintf(v,"%02x", i);
    strcat(cache_path, v);
  }
  strcat(cache_path, ".jpeg");
}
/**
 * Checks if a cache file exists and is still current
 * @param path
 * @param cache_path
 * @return true if cache file is valid, otherwise false
 */
bool check_cache(const char *path, const char *cache_path) {
  if (is_file(cache_path)) {
    struct stat original{};
    stat(path, &original);
    struct stat cache{};
    stat(cache_path, &cache);
    if (original.st_mtime != cache.st_mtime) {
      return false;
    }
    return true;
  }
  return false;
}

static image::Image *openFile(const char *path, hal::display::Display *display) {
  image::Image *in = nullptr;
  char cache_path[128] = "";
  get_cache(path, cache_path);
  if (check_cache(path, cache_path)) {
    in = ImageFactory(get_board()->GetDisplay()->size, cache_path);
  }
  else {
    in = ImageFactory(get_board()->GetDisplay()->size, path);
  }
  if (in) {
    if (in->Open(get_board()->TurboBuffer()) != 0) {
      const char *lastError = in->GetLastError();
      delete in;
      return new image::ErrorImage(display->size, "Error Displaying File\n%s\n%s", path, lastError);
    }
    printf("%s x: %i y: %i\n", path, in->Size().first, in->Size().second);
    auto size = in->Size();
    if (size > display->size && in->resizable() == false) {
      delete in;
      return new image::TooLargeImage(display->size, path);
    }
    if(size > display->size && in->resizable() == true) {
      int64_t start = millis();
      auto *resizing = new image::ResizingImage(display->size);
      resizing->GetFrame(display->buffer, 0, 0);
      display->write(0, 0, display->size.first, display->size.second, display->buffer);
      delete resizing;
      memset(display->buffer, 0, display->size.first*display->size.second*2);
      if (in->resize(display->buffer, 0, 0, display->size.first, display->size.second) != 0) {
        delete in;
        return new image::ErrorImage(display->size, "Error Resizing File\n%s", path);
      }
      if (save_cache(path, cache_path, display->buffer) != 0) {
        return new image::ErrorImage(display->size, "Error Saving Resized Image\n%s", path);
      }
      LOGI(TAG, "Resizing time %lld", millis()-start);

      delete in;
      in = ImageFactory(get_board()->GetDisplay()->size, cache_path);
      if (in->Open(get_board()->TurboBuffer()) != 0) {
        const char *lastError = in->GetLastError();
        delete in;
        return new image::ErrorImage(display->size, "Error Displaying File\n%s\n%s", path, lastError);
      }
    }
  } else {
    return new image::ErrorImage(display->size, "Unsupported File\n%s", path);
  }
  newImage = true;
  frame_count = 0;
  average_frame_delay = 0;
  max_frame_delay = 0;
  last_fps = 0;
  looped = false;
  image_start = millis();
  return in;
}

static image::Image *openFileUpdatePath(char *path, hal::display::Display *display) {
  if (get_file(path) != 0) {
    file_position = -1;
    return new image::NoImage(display->size);
  }
  return openFile(path, display);
}

static void next_prev(std::unique_ptr<image::Image> &in, char *current_file, hal::config::Config *config, hal::display::Display *display, int increment){
  if (config->getLocked() || file_position < 0) {
    return;
  }
  const char *next = directory_get_increment(&dir, file_position, increment);
  LOGI(TAG, "Next %s", next);
  if (next != nullptr) {
    strcpy(basename(current_file), next);
    in.reset(); //Free the memory before trying to open the next image
    in.reset(openFileUpdatePath(current_file, display));
  }
}

TimerHandle_t slideShowTimer = nullptr;

static void slideShowHandler(TimerHandle_t) {
  TaskHandle_t displayHandle = xTaskGetHandle("display_task");
  xTaskNotifyIndexed(displayHandle, 0, DISPLAY_ADVANCE, eSetValueWithOverwrite);
}

static void slideShowStart(hal::config::Config *config) {
  if (config->getSlideShow()) {
    xTimerChangePeriod(slideShowTimer, (config->getSlideShowTime() * 1000) / portTICK_PERIOD_MS, 0);
    xTimerStart(slideShowTimer, 0);
  } else {
    xTimerStop(slideShowTimer, 0);
  }
}

static void slideShowRestart() {
  if (xTimerIsTimerActive(slideShowTimer) != pdFALSE) {
    xTimerReset(slideShowTimer, 50 / portTICK_PERIOD_MS);
  }
}

static void slideShowStop() {
  if (xTimerIsTimerActive(slideShowTimer) != pdFALSE) {
    xTimerStop(slideShowTimer, 50 / portTICK_PERIOD_MS);
  }
}

void display_task(void *params) {
  auto *board = static_cast<Boards::Board *>(params);

  auto config = board->GetConfig();

  auto display = board->GetDisplay();

  slideShowTimer = xTimerCreate("slideshow", 100 / portTICK_PERIOD_MS, pdTRUE, nullptr, slideShowHandler);

  memset(display->buffer, 255, display->size.first * display->size.second * 2);
  display->write(0, 0, display->size.first, display->size.second, display->buffer);

  board->GetBacklight()->setLevel(board->GetConfig()->getBacklight() * 10);

  LOGI(TAG, "Display Resolution %ix%i", display->size.first, display->size.second);

  int32_t delay = 1000/portTICK_PERIOD_MS; //Delay for display loop. Is adjusted by the results of the loop method of the image being displayed
  bool redraw = false; //Reload from configuration next time we go to display an image
  bool advance = false; //Advance the slideshow
  image::frameStatus endOfFrame = image::frameStatus::ERROR; //Last frame of animation
  std::unique_ptr<image::Image> in = nullptr; //The image we are displaying
  char current_file[MAX_FILE_LEN + 1]; //The current file path that has been selected
  config->getPath(current_file);
  char card_path[MAX_FILE_LEN + 1];
  uint32_t option = 0;
  TaskHandle_t lvglTask = xTaskGetHandle("LVGL");

  while (true) {
    option = 0;
    if (xTaskNotifyWaitIndexed(0, 0, 0xffffffff, &option, delay == -1 ? portMAX_DELAY : delay)) {
      LOGD(TAG, "Woken up id: %d, Reset: %s", option & ~noResetBit, !(option & noResetBit)?"True":"False");
      if (option != DISPLAY_NONE) {
        config->reload();
        if (!(option & noResetBit)) {
          in.reset();
        }
        switch (option) {
          case DISPLAY_FILE:
            LOGD(TAG, "DISPLAY_FILE");
            if (!valid_image_file(current_file, extensions)) {
              config->getPath(current_file);
            }
            in.reset(openFileUpdatePath(current_file, display));
            slideShowStart(config);
            break;
          case DISPLAY_NEXT:
            LOGD(TAG, "DISPLAY_NEXT");
            next_prev(in, current_file, config, display, 1);
            slideShowRestart();
            break;
          case DISPLAY_PREVIOUS:
            LOGD(TAG, "DISPLAY_PREVIOUS");
            next_prev(in, current_file, config, display, -1);
            slideShowRestart();
            break;
          case DISPLAY_SPECIAL_1:
            LOGD(TAG, "DISPLAY_SPECIAL_1");
            get_board()->GetConfig()->getCard(hal::config::cards::UP, card_path);
            if (is_file(card_path)) {
              in.reset(openFile(card_path, display));
              slideShowStop();
            } else {
              in.reset();
              in = std::make_unique<image::ErrorImage>(display->size, "Card 1 not set");
            }
            break;
          case DISPLAY_SPECIAL_2:
            LOGD(TAG, "DISPLAY_SPECIAL_2");
            get_board()->GetConfig()->getCard(hal::config::cards::DOWN, card_path);
            if (is_file(card_path)) {
              in.reset(openFile(card_path, display));
              slideShowStop();
            } else {
              in.reset();
              in = std::make_unique<image::ErrorImage>(display->size, "Card 2 not set");
            }
            break;
          case DISPLAY_NOTIFY_CHANGE:
            redraw = true;
            // Something has changed in the configuration, reopen the configured file.
            config->getPath(current_file);
            delay = -1;
            continue;
          case DISPLAY_NOTIFY_USB:
            LOGD(TAG, "DISPLAY_NOTIFY_USB");
            slideShowStop();
            delay = -1;
            closedir_sorted(&dir);
            dir.dirptr = nullptr;
            xTaskNotifyIndexed(lvglTask, 0, 0, eSetValueWithOverwrite);
            continue;
          case DISPLAY_ADVANCE:
            advance = true;
            break;
          case DISPLAY_STOP:
            xTimerDelete(slideShowTimer, 0);
            in.reset();
            vTaskDelete(nullptr);
            return;
          case DISPLAY_MENU:
            delay = -1;
            LOGD("TAG", "Display menu");
            slideShowStop();
            xTaskNotifyIndexed(lvglTask, 0, 0, eSetValueWithOverwrite);
            continue;
          default:
            break;
        }
      }
    }

    if (redraw) {
      in.reset();
      in.reset(openFileUpdatePath(current_file, display));
      slideShowStart(config);
      redraw = false;
      advance = false;
      endOfFrame = image::frameStatus::OK;
    }

    if (advance && endOfFrame == image::frameStatus::END) {
      LOGD(TAG, "advance");
      next_prev(in, current_file, config, display, 1);
      advance = false;
      endOfFrame = image::frameStatus::OK;
    }

    if (in) {
      // If there is an open file, display the next frame
      std::tie(endOfFrame, delay) = displayFile(in, display);
      if (endOfFrame == image::frameStatus::ERROR) {
        if (config->getSlideShow()) {
          //Skip the error, and head to the next file in slideshow mode
          next_prev(in, current_file, config, display, 1);
          continue;
        }
        in = std::make_unique<image::ErrorImage>(display->size,
                                                 "Error Displaying File\n%s\n%s",
                                                 current_file,
                                                 in->GetLastError());
      }
    }
  }
}