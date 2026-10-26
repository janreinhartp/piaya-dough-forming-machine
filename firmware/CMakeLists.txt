cmake_minimum_required(VERSION 3.16)

set(DEMO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../docs/demo-firmware/Demo")
if(NOT EXISTS "${DEMO_ROOT}/components/lvgl_port/CMakeLists.txt")
    message(FATAL_ERROR "The local Waveshare sample is missing at docs/demo-firmware/Demo")
endif()
set(EXTRA_COMPONENT_DIRS "${DEMO_ROOT}/components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# Build main and its requirements only. The local sample directory also contains
# SD, Wi-Fi, and demo UI components that this controller does not use.
idf_build_set_property(MINIMAL_BUILD ON)
project(piaya_dough_maker)
