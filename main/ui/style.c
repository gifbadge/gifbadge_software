/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include "log.h"
#include "ui/style.h"

#ifdef __cplusplus
extern "C"
{
#endif

lv_style_t icon_style;
lv_style_t menu_font_style;

const lv_style_const_prop_t file_select_style_props[] = {
    LV_STYLE_CONST_BG_COLOR(LV_COLOR_MAKE(40, 43, 48)),
    LV_STYLE_CONST_BG_OPA(255),
    LV_STYLE_CONST_BORDER_COLOR(LV_COLOR_MAKE(47, 50, 55)),
    LV_STYLE_CONST_BORDER_SIDE(LV_BORDER_SIDE_FULL),
    LV_STYLE_CONST_RADIUS(7),
    LV_STYLE_CONST_PAD_BOTTOM(10),
    LV_STYLE_CONST_PAD_TOP(10),
    LV_STYLE_CONST_PAD_LEFT(10),
    LV_STYLE_CONST_PAD_RIGHT(10),
    LV_STYLE_CONST_PROPS_END
};

LV_STYLE_CONST_INIT(file_select_style, file_select_style_props);

const lv_style_const_prop_t style_battery_bar_props[] = {
    LV_STYLE_CONST_BG_OPA(LV_OPA_COVER),
    LV_STYLE_CONST_BG_COLOR(LV_COLOR_MAKE(0,0,0)),
    LV_STYLE_CONST_PAD_BOTTOM(5),
    LV_STYLE_CONST_PAD_TOP(5),
    LV_STYLE_CONST_PAD_LEFT(5),
    LV_STYLE_CONST_PAD_RIGHT(5),
    LV_STYLE_CONST_PROPS_END
};

LV_STYLE_CONST_INIT(style_battery_bar, style_battery_bar_props);

const lv_style_const_prop_t style_battery_indicator_props[] = {
    LV_STYLE_CONST_BG_OPA(LV_OPA_COVER),
    LV_STYLE_CONST_BG_COLOR(LV_COLOR_MAKE(0,0,0)),
    LV_STYLE_CONST_PROPS_END
};

LV_STYLE_CONST_INIT(style_battery_indicator, style_battery_indicator_props);

const lv_style_const_prop_t style_battery_main_props[] = {
    LV_STYLE_CONST_BG_OPA(LV_OPA_COVER),
    LV_STYLE_CONST_BG_COLOR(LV_COLOR_MAKE(255,255,255)),
    LV_STYLE_CONST_PAD_BOTTOM(5),
    LV_STYLE_CONST_PAD_TOP(5),
    LV_STYLE_CONST_PAD_LEFT(5),
    LV_STYLE_CONST_PAD_RIGHT(5),
    LV_STYLE_CONST_PROPS_END
};

LV_STYLE_CONST_INIT(style_battery_main, style_battery_main_props);

const lv_style_const_prop_t style_battery_icon_props[] = {
    LV_STYLE_CONST_TEXT_FONT(&battery_symbols_14),
    LV_STYLE_CONST_TEXT_ALIGN(LV_TEXT_ALIGN_CENTER),
    LV_STYLE_CONST_PROPS_END
};

LV_STYLE_CONST_INIT(style_battery_icon, style_battery_icon_props);

const lv_style_const_prop_t style_battery_icon_container_props[] = {
    LV_STYLE_CONST_BG_OPA(LV_OPA_COVER),
    LV_STYLE_CONST_BG_COLOR(LV_COLOR_MAKE(0,0,0)),
    LV_STYLE_CONST_RADIUS(LV_RADIUS_CIRCLE),
    LV_STYLE_CONST_PROPS_END
};

LV_STYLE_CONST_INIT(style_battery_icon_container, style_battery_icon_container_props);

const lv_style_const_prop_t container_style_props[] = {
    LV_STYLE_CONST_BG_OPA(LV_OPA_TRANSP),
    LV_STYLE_CONST_PAD_BOTTOM(0),
    LV_STYLE_CONST_PAD_TOP(0),
    LV_STYLE_CONST_PAD_LEFT(0),
    LV_STYLE_CONST_PAD_RIGHT(0),
    LV_STYLE_CONST_BORDER_SIDE(LV_BORDER_SIDE_NONE),
    LV_STYLE_CONST_PROPS_END
};

LV_STYLE_CONST_INIT(container_style, container_style_props);



void style_init() {
  lv_style_init(&icon_style);
  lv_style_init(&menu_font_style);


  lv_style_set_text_color(&icon_style, lv_color_white());
  if (lv_disp_get_hor_res(NULL) > 240) {
    lv_style_set_text_font(&icon_style, &material_icons_56);
    lv_style_set_text_font(&menu_font_style, &lv_font_montserrat_28_compressed);
  } else {
    lv_style_set_text_font(&icon_style, &material_icons);
    lv_style_set_text_font(&menu_font_style, &lv_font_montserrat_14);
  }
}

#ifdef __cplusplus
}
#endif