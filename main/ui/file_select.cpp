/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include <dirent.h>
#include <cstring>
#include "ui/file_select.h"
#include "ui/widgets/file_list/file_list.h"
#include "ui/device_group.h"
#include "ui/style.h"
#include "ui/menu.h"
#include "directory.h"
#include "dirname.h"
#include "file.h"

extern "C" {

struct file_data {
  char top[255];
  char current[255];
  bool allow_folder;
};

static void file_list_chdir(lv_obj_t *parent, const char *path) {
  LV_LOG_USER("chdir Path: %s", path);
  auto *d = static_cast<file_data *>(lv_file_list_get_user_data(parent));
  if(d->current != path){
    strcpy(d->current, path);
  }
  lv_file_list_clear(parent);
  file_list(parent);
  lv_file_list_scroll_to_view(parent, 0);
}

static void file_event_handler(lv_event_t *e) {
  auto *container = static_cast<lv_obj_t *>(lv_event_get_user_data(e));
  auto *d = static_cast<file_data *>(lv_file_list_get_user_data(container));

  auto *obj = static_cast<lv_obj_t *>(lv_event_get_target(e));
  char *text = lv_label_get_text(obj);

  char new_path[256];

  LV_LOG_USER("Clicked: %s %s", d->current, text);
  if (strcmp(text, "Up") == 0) {
    dirname(d->current);
    file_list_chdir(container, d->current);
  } else if (strcmp(text, "Entire Folder") == 0) {
    if (!is_directory(d->current)) {
      dirname(d->current);
    }
    lv_obj_del(container);
  } else if (strcmp(text, "Exit") == 0) {
    lv_obj_del(container);
  } else {
    if (!is_directory(d->current)) {
      dirname(d->current);
    }
    snprintf(new_path, sizeof(new_path), "%s/%s", d->current, text);
    LV_LOG_USER("Save Path: %s", new_path);
    if (is_directory(new_path)) {
      file_list_chdir(container, new_path);
    } else if (is_file(new_path)) {
      strcpy(d->current, new_path);
      lv_obj_del(container);
    }
  }
}

static lv_obj_t *file_entry(lv_obj_t *parent, const char *icon, const char *text) {
  lv_obj_t *btn = lv_file_list_add(parent, icon);
  lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_t *label = lv_label_create(btn);
  lv_obj_add_style(label, &menu_font_style, LV_PART_MAIN);
  lv_label_set_text(label, text);
  lv_obj_add_event_cb(label, file_event_handler, LV_EVENT_CLICKED, parent);
  return btn;
}

void file_list(lv_obj_t *parent) {
  auto *d = static_cast<file_data *>(lv_file_list_get_user_data(parent));
  char *top = d->top;
  char *current = d->current;

  if(!is_directory(current)){
    current = dirname(current);
  }

  if (compare_path(top, current) != 0) {
    LV_LOG_USER("Not in Top");
    file_entry(parent, ICON_UP, "Up");
  }
  if (d->allow_folder) {
    file_entry(parent, ICON_FOLDER_OPEN, "Entire Folder");
  } else if (compare_path(top, current) == 0) {
    file_entry(parent, ICON_BACK, "Exit");
  }

  LV_LOG_USER("Path: %s", current);

  DIR_SORTED dir;
  struct dirent *ep;
  if (opendir_sorted(&dir, current, nullptr)) {
    while ((ep = readdir_sorted(&dir)) != nullptr) {
      if (ep->d_name[0] == '.')
        continue;
      if(strcasecmp(ep->d_name, "System Volume Information") == 0){
        continue;
      }
      if (ep->d_type == DT_DIR)
        file_entry(parent, ICON_FOLDER, ep->d_name);
    }

  rewinddir_sorted(&dir);

    while ((ep = readdir_sorted(&dir)) != nullptr) {
      if (ep->d_name[0] == '.')
        continue;
      if (ep->d_type == DT_REG)
        file_entry(parent, ICON_IMAGE, ep->d_name);
    }
    closedir_sorted(&dir);
  }
}

lv_obj_t *file_select(const char *top, const char *current, bool allow_folder) {
  lv_screen_load(create_screen());
  new_group();
  lv_obj_t *cont_flex = lv_file_list_create(lv_scr_act());
  lv_file_list_icon_style(cont_flex, &icon_style);

  auto *d = static_cast<file_data *>(malloc(sizeof(file_data)));
  if (strlen(current) == 0) {
    strcpy(d->current, top);
  }
  else {
    strcpy(d->current, current);
  }
  strcpy(d->top, top);
  d->allow_folder = allow_folder;

  lv_file_list_set_user_data(cont_flex, d);

  file_list(cont_flex);

  lv_file_list_scroll_to_view(cont_flex, 0);
  return cont_flex;
}

void FileWindowClose(lv_event_t *e) {
  auto *file_window = static_cast<lv_obj_t *>(lv_event_get_target(e));
  auto *file_widget = static_cast<lv_obj_t *>(lv_event_get_user_data(e));
  auto *d = static_cast<file_data *>(lv_file_list_get_user_data(file_window));
  LV_LOG_USER("File: %s", d->current);
  lv_label_set_text(file_widget, d->current);
  free(lv_file_list_get_user_data(file_window));
  restore_group(lv_obj_get_parent(file_widget));
  lv_screen_load(lv_obj_get_screen(file_widget));
  lv_obj_scroll_to_view(static_cast<lv_obj_t *>(lv_event_get_user_data(e)), LV_ANIM_OFF);
}
}