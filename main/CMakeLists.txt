SET(SRC "main.cpp"
        "ui/menu.cpp"
        "display.cpp"
        "input.cpp"
        ${CMAKE_SOURCE_DIR}/main/ui/fonts/material_icons.c
        ${CMAKE_SOURCE_DIR}/main/ui/fonts/material_icons_56.c
        ${CMAKE_SOURCE_DIR}/main/ui/fonts/material_icons_special.c
        ${CMAKE_SOURCE_DIR}/main/ui/fonts/battery_symbols_14.c
        "ui/menu.cpp"
        "ui/device_group.c"
        "ui/file_select.cpp"
        "ui/main_menu.cpp"
        "ui/file_options.cpp"
        "ui/widgets/file_list/file_list.c"
        "ui/widgets/battery/lv_battery.c"
        "ui/style.c"
        "ui/storage.cpp"
        "ui/usb_connected.cpp"
        "ui/device_info.cpp"
        "ui/help.cpp"
        "ui/battery.cpp"
        "ui/ota.cpp"
        "ui/no_sd.cpp"
        "ui/low_batt.cpp"
        "ui/reset_settings.cpp"
        "resize.cpp"
)

if (ESP_PLATFORM)
    idf_component_register(SRCS
            ${SRC}
            INCLUDE_DIRS "include" "${CMAKE_SOURCE_DIR}/external-deps/JPEGENC/src"
            REQUIRES gifbadge_hal_esp32 lvgl esp_timer freertos)
    target_compile_options(${COMPONENT_LIB} PRIVATE -D__LINUX__)
    target_link_libraries(${COMPONENT_TARGET} PRIVATE FileUtil FontRender FileBuffer EmbeddedImage gifbadge_hal)

    #
else ()
    add_executable(GifBadge ${SRC})
    target_include_directories(GifBadge PRIVATE include "${CMAKE_SOURCE_DIR}/external-deps/JPEGENC/src")
endif ()

set(font_symbols "0xe3f4,0xe897,0xe41b,0xe41b,0xe425,0xe161,0xe5c9,0xe3ab,0xe2c7,0xe2c8,0xe623,0xf80e,0xf568,0xe5d8,0xf182,0xf181")
message(font_symbols="${font_symbols}")

set(special_symbols "0xe1e0,0xe3f4,0xe8d7,0xf057,0xe19c")
message(special_symbols="${special_symbols}")

set(battery_symbols "0xea0b,0xf22f,0xe5cd") #Charging,!, X


find_program(LV_FONT_CONV "lv_font_conv")
if (LV_FONT_CONV)
    add_custom_command(
            OUTPUT ${CMAKE_SOURCE_DIR}/main/ui/fonts/material_icons.c
            COMMAND ${LV_FONT_CONV} --bpp 1 --size 28 --format lvgl --lv-include lvgl.h --font ${CMAKE_SOURCE_DIR}/MaterialSymbolsOutlined[FILL,GRAD,opsz,wght].ttf -r"${font_symbols}" --no-compress -o ${CMAKE_SOURCE_DIR}/main/ui/fonts/material_icons.c
    )

    add_custom_command(
            OUTPUT ${CMAKE_SOURCE_DIR}/main/ui/fonts/material_icons_56.c
            COMMAND ${LV_FONT_CONV}  --bpp 1 --size 56 --format lvgl --lv-include lvgl.h --font ${CMAKE_SOURCE_DIR}/MaterialSymbolsOutlined[FILL,GRAD,opsz,wght].ttf -r"${font_symbols}" -o ${CMAKE_SOURCE_DIR}/main/ui/fonts/material_icons_56.c
    )

    add_custom_command(
            OUTPUT ${CMAKE_SOURCE_DIR}/main/ui/fonts/battery_symbols_14.c
            COMMAND ${LV_FONT_CONV}  --bpp 1 --size 14 --format lvgl --lv-include lvgl.h --font ${CMAKE_SOURCE_DIR}/MaterialSymbolsOutlined[FILL,GRAD,opsz,wght]-Filled.ttf -r"${battery_symbols}" --no-compress -o ${CMAKE_SOURCE_DIR}/main/ui/fonts/battery_symbols_14.c
    )

    add_custom_command(
            OUTPUT ${CMAKE_SOURCE_DIR}/main/ui/fonts/material_icons_special.c
            COMMAND ${LV_FONT_CONV}  --bpp 1 --size 180 --format lvgl --lv-include lvgl.h --font ${CMAKE_SOURCE_DIR}/MaterialSymbolsOutlined[FILL,GRAD,opsz,wght].ttf -r"${special_symbols}" -o ${CMAKE_SOURCE_DIR}/main/ui/fonts//material_icons_special.c
    )
endif ()