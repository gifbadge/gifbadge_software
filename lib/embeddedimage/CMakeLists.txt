cmake_minimum_required(VERSION 3.1...3.29)
project(EmbeddedImage VERSION 1.0 LANGUAGES C CXX ASM)

file(GLOB PNGdec_src ${CMAKE_SOURCE_DIR}/external-deps/PNGdec/src/*.c ${CMAKE_SOURCE_DIR}/external-deps/PNGdec/src/*.cpp)
file(GLOB JPEGDEC_src ${CMAKE_SOURCE_DIR}/external-deps/JPEGDEC/src/*.c ${CMAKE_SOURCE_DIR}/external-deps/JPEGDEC/src/*.cpp ${CMAKE_SOURCE_DIR}/external-deps/JPEGDEC/src/*.S)

set(ESP_COMPILE_OPTIONS -DNDEBUG -ffunction-sections -fdata-sections -O2 -Wwrite-strings $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions -fno-rtti -fuse-cxa-atexit>)

add_library(PNGdec ${PNGdec_src})
target_include_directories(PNGdec PUBLIC "${CMAKE_SOURCE_DIR}/external-deps/PNGdec/src")
target_compile_options(PNGdec PRIVATE -D__LINUX__)
if (ESP_PLATFORM)
    target_compile_options(PNGdec PRIVATE -DARDUINO_ARCH_ESP32=1)
    target_link_libraries(PNGdec PRIVATE idf::newlib)
    target_link_libraries(PNGdec PRIVATE idf::espressif__esp-dsp)
    target_compile_options(PNGdec PUBLIC ${ESP_COMPILE_OPTIONS})
    target_link_options(PNGdec    PUBLIC ${ESP_COMPILE_OPTIONS})
else()
    target_precompile_headers(PNGdec PUBLIC <stdint.h>) # Work around this being missing in PNGdec's includes
endif ()

add_library(JPEGDEC ${JPEGDEC_src})
target_include_directories(JPEGDEC PUBLIC "${CMAKE_SOURCE_DIR}/external-deps/JPEGDEC/src")
target_compile_options(JPEGDEC PRIVATE -D__LINUX__)
if (ESP_PLATFORM)
    target_compile_options(JPEGDEC PRIVATE -DARDUINO_ARCH_ESP32=1)
    target_link_libraries(JPEGDEC PRIVATE idf::newlib)
    target_link_libraries(JPEGDEC PRIVATE idf::espressif__esp-dsp)
    target_compile_options(JPEGDEC PUBLIC ${ESP_COMPILE_OPTIONS})
    target_link_options(JPEGDEC    PUBLIC ${ESP_COMPILE_OPTIONS})
endif ()

SET(SRC "image.cpp" "bitbank2.cpp" "jpeg.cpp" "png.cpp" "gif.cpp" "bmp.cpp" "resize.cpp" ${PNGdec_src} )

SET(INCLUDE "include"
        "${CMAKE_SOURCE_DIR}/external-deps/AnimatedGIF/src"
)

add_library(EmbeddedImage ${SRC} ${INCLUDE})
target_include_directories(EmbeddedImage PUBLIC ${INCLUDE})
target_compile_options(EmbeddedImage PUBLIC -D__LINUX__)

target_link_libraries(EmbeddedImage PRIVATE FileBuffer)
target_link_libraries(EmbeddedImage PUBLIC PNGdec JPEGDEC simplebmp)

set_property(TARGET EmbeddedImage PROPERTY C_STANDARD 11)
set_property(TARGET EmbeddedImage PROPERTY C_EXTENSIONS ON)
set_property(TARGET EmbeddedImage PROPERTY CXX_STANDARD 23)
set_property(TARGET EmbeddedImage PROPERTY CXX_EXTENSIONS ON)

if (ESP_PLATFORM)
    target_link_libraries(EmbeddedImage PRIVATE idf::newlib)
    target_compile_definitions(EmbeddedImage PUBLIC ESP_PLATFORM=1)
    add_compile_definitions(ALLOWS_UNALIGNED)
    target_compile_options(EmbeddedImage PRIVATE -D__LINUX__ -DARDUINO_ARCH_ESP32=1)
    target_compile_options(EmbeddedImage PUBLIC ${ESP_COMPILE_OPTIONS})
    target_link_options(EmbeddedImage    PUBLIC ${ESP_COMPILE_OPTIONS})
else()
    include(CTest)
    add_subdirectory(test)
    target_compile_options(EmbeddedImage PUBLIC --coverage)
endif ()