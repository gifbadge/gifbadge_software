/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include <cstdlib>
#include "drivers/display_sdl.h"

#include <cstdio>

#include "log.h"
#include "mask.h"

hal::display::oslinux::display_sdl *displaySdl = nullptr;

hal::display::oslinux::display_sdl::display_sdl(const std::pair<uint16_t, uint16_t> &_size) {
  sem_init(&mutex, 0, 0);
  size = _size;
  buffer = static_cast<uint8_t *>(malloc(size.first*size.second*2));
  if (!SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS)) {
    LOGI("hal::display::oslinux::display_sdl", "error initializing SDL: %s\n", SDL_GetError());
  }

  win = SDL_CreateWindow("GAME",size.first, size.second, 0);

  renderer = SDL_CreateRenderer(win, nullptr);
  SDL_SetRenderDrawColor(renderer, 255, 255,
                         255, 255);
  SDL_RenderClear(renderer);
  pixels = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, size.first, size.second);
  mask = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 480, 480);
  void *data;
  int pitch;
  SDL_LockTexture(mask, nullptr, &data, &pitch);

  memcpy(data, circle_mask.pixel_data, 480 * 480 * 4);

  SDL_UnlockTexture(mask);


}
hal::display::oslinux::display_sdl::~display_sdl() {
  SDL_DestroyTexture(pixels);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(win);
  SDL_Quit();
  if (buffer != nullptr) {
    free(buffer);
  }
  sem_destroy(&mutex);
}
bool hal::display::oslinux::display_sdl::onColorTransDone(flushCallback_t callback) {
  _callback = callback;
  return false;
}
void hal::display::oslinux::display_sdl::write(int x_start, int y_start, int x_end, int y_end, void *color_data) {
  if (color_data != buffer) {
    memcpy(buffer, color_data, size.first*size.second*2);
  }
  sem_post(&mutex);
}
void hal::display::oslinux::display_sdl::update() {
  if(sem_trywait(&mutex) != -1) {
    void *data;
    int pitch;
    SDL_LockTexture(pixels, nullptr, &data, &pitch);

    memcpy(data, buffer, size.first*size.second*2);

    SDL_UnlockTexture(pixels);

    // copy to window
    SDL_RenderTexture(renderer, pixels, nullptr, nullptr);
    SDL_RenderTexture(renderer, mask, nullptr, nullptr);
    SDL_RenderPresent(renderer);
    if (_callback) {
      _callback();
    }
  }
}

void hal::display::oslinux::display_sdl::clear() {

}

hal::display::oslinux::display_sdl *display_sdl_init(const std::pair<uint16_t, uint16_t> &_size) {
  displaySdl = new hal::display::oslinux::display_sdl(_size);
  return displaySdl;
}
