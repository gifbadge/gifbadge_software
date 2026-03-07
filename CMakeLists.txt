cmake_minimum_required(VERSION 3.24)

if (ESP_PLATFORM)
    set(COMPONENT_DIRS "$ENV{IDF_PATH}/components" "${CMAKE_SOURCE_DIR}/idf-components" "${CMAKE_SOURCE_DIR}/managed_components" "${CMAKE_SOURCE_DIR}/main")
    set(COMPONENTS "esp32" "esptool_py" "main" "log")
    include($ENV{IDF_PATH}/tools/cmake/project.cmake)
endif ()

project(GifBadge LANGUAGES C CXX)

if(NOT ESP_PLATFORM)
    #FreeRTOS stuff
    set( NO_TRACING 1 )
    add_compile_options( -DprojENABLE_TRACING=0 )
    add_compile_options( -DprojCOVERAGE_TEST=0 )

    if( PROFILE )
        set( CMAKE_BUILD_TYPE "release" )
    else()
        set( CMAKE_BUILD_TYPE "debug" )
    endif()

    if( SANITIZE_ADDRESS )
        #        add_compile_options( -fsanitize=address -fsanitize=alignment )
        #    add_link_options(-fsanitize=address -fsanitize=alignment)
        add_compile_options( -fsanitize=address  )
        add_link_options(-fsanitize=address)
    endif()

    if( SANITIZE_LEAK )
        add_compile_options( -fsanitize=leak )
    endif()

#    if(COVERAGE)
        add_compile_options(--coverage)
        add_link_options(--coverage)
#    endif ()

    # Add the freertos_config for FreeRTOS-Kernel
    add_library( freertos_config INTERFACE )

    target_include_directories( freertos_config
            INTERFACE
            "${CMAKE_SOURCE_DIR}/config/linux/")

    # Select the heap port
    set( FREERTOS_HEAP "3" CACHE STRING "" FORCE)

    # Select the native compile PORT
    set( FREERTOS_PORT "GCC_POSIX" CACHE STRING "" FORCE)

    set(FREERTOS_CONFIG_FILE_DIRECTORY "${CMAKE_SOURCE_DIR}/config/linux/" CACHE STRING "")
    # Adding the FreeRTOS-Kernel subdirectory
    add_subdirectory(external-deps/FreeRTOS-Kernel)

    target_compile_options(freertos_kernel
            PRIVATE
            # Trace macro cast pointer to int to store memory management event
            $<IF:${NO_TRACING},,-Wno-pointer-to-int-cast>
    )
    #End of FreeRTOS stuff
    enable_testing()
endif ()

add_subdirectory(lib/gifbadge_hal)
add_subdirectory(lib/fileutil)
add_subdirectory(lib/font_render)
add_subdirectory(external-deps/npmx)
add_subdirectory(lib/filebuffer)
add_subdirectory(lib/embeddedimage)
add_subdirectory(lib/simplebmp)
add_subdirectory(lib/zmk_debounce)

if (ESP_PLATFORM)
    if(EXISTS ${CMAKE_SOURCE_DIR}/config/esp32s3/lv_conf.h)
        message(LV_CONF_PATH=${CMAKE_SOURCE_DIR}/config/esp32s3/lv_conf.h)
        idf_build_set_property(COMPILE_OPTIONS "-I${CMAKE_SOURCE_DIR}/config/esp32s3" APPEND)
    endif ()
else()
    add_compile_options( -D__linux__)

    find_package(SDL3 REQUIRED)
    include_directories(${SDL3_INCLUDE_DIRS})

    add_subdirectory(main)
    add_subdirectory(lib/gifbadge_hal_linux)

    message(LV_CONF_PATH="${CMAKE_SOURCE_DIR}/config/linux/lv_conf.h")
    add_subdirectory(external-deps/lvgl)
    target_compile_options(lvgl PUBLIC -I${CMAKE_SOURCE_DIR}/config/linux/)

    target_link_libraries(GifBadge gifbadge_hal gifbadge_hal_linux freertos_kernel EmbeddedImage FileUtil FontRender FileBuffer lvgl ${SDL3_LIBRARIES})
    set_property(TARGET GifBadge PROPERTY CXX_STANDARD 23)
    target_compile_options(GifBadge PRIVATE -fanalyzer)
    add_library(unity STATIC ${CMAKE_SOURCE_DIR}/external-deps/Unity/src/unity.c)
    target_include_directories(unity PUBLIC ${CMAKE_SOURCE_DIR}/external-deps/Unity/src/)
    set(C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 23)
    set(C_EXTENSIONS ON)
    set(CXX_EXTENSIONS ON)
endif ()
