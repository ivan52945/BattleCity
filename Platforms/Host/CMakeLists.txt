# Platforms/Host - the host platform of the typed core: the same
# GameT instantiations as the MCU platforms, over the SimLcd fake
# panels instead of ST7789/ST7735. Windows, mingw g++ (or clang).
#
# NOT linked against the Game_Core CMake targets on purpose: the
# `display` target links stm32cubemx (HAL headers) - the host build
# compiles Game/Src/assets.cpp directly, with the same three include
# paths the firmware toolchain uses.
#
# Configure: cmake -S . -B build -G Ninja
# Build:     cmake --build build
# Artifacts: build/host_view.exe (batch dumps), build/host_view_live.exe
#            (live window), build/wingrab.exe (window grab).
# -static is mandatory: a foreign libstdc++-6.dll on PATH catches
# STATUS_ENTRYPOINT_NOT_FOUND at load time.
cmake_minimum_required(VERSION 3.20)
project(HostView CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../Game_Core)

set(host_inc ${CORE_DIR}/Game/Inc
             ${CORE_DIR}/Utils/Inc
             ${CORE_DIR}/DeviceDrivers/Display/Inc)
set(core_src ${CORE_DIR}/Game/Src/assets.cpp)

add_executable(host_view main.cpp sim_lcd.hpp png.hpp ${core_src})
target_include_directories(host_view PRIVATE ${host_inc})
target_compile_options(host_view PRIVATE -Wall -Wextra)
target_link_options(host_view PRIVATE -static)

if(WIN32)
    add_executable(host_view_live live.cpp sim_lcd.hpp png.hpp ${core_src})
    target_include_directories(host_view_live PRIVATE ${host_inc})
    target_compile_options(host_view_live PRIVATE -Wall -Wextra)
    target_link_options(host_view_live PRIVATE -static)
    target_link_libraries(host_view_live PRIVATE gdi32 user32)

    add_executable(wingrab wingrab.cpp png.hpp)
    target_compile_options(wingrab PRIVATE -Wall -Wextra)
    target_link_options(wingrab PRIVATE -static)
    target_link_libraries(wingrab PRIVATE gdi32 user32)
endif()
