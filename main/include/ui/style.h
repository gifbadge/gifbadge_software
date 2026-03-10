/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif
#include <lvgl.h>
LV_FONT_DECLARE(material_icons)
LV_FONT_DECLARE(material_icons_56)
LV_FONT_DECLARE(material_icons_special)
LV_FONT_DECLARE(battery_symbols_14)


#define ICON_FOLDER "\ue2c7"
#define ICON_IMAGE "\ue3f4"
#define ICON_FOLDER_OPEN "\ue2c8"
#define ICON_BACK "\ue166"
#define ICON_UP "\ue5d8"

#define ICON_USB "\ue1e0"
#define ICON_UPDATE "\ue8d7"
#define ICON_SD_ERROR "\uf057"
#define ICON_LOW_BATTERY "\ue19c"

extern lv_style_t icon_style;
extern lv_style_t menu_font_style;
extern const lv_style_t container_style;
extern const lv_style_t file_select_style;
extern const lv_style_t style_battery_bar;
extern const lv_style_t style_battery_indicator;
extern const lv_style_t style_battery_main;
extern const lv_style_t style_battery_icon;
extern const lv_style_t style_battery_icon_container;


void style_init();

#ifdef __cplusplus
}
#endif