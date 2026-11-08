cmake_minimum_required(VERSION 3.22)

SET(TARGET_LIB display)

# Single display module. Both ST7735/ and ST7789/ folders are merged into
# it now - every driver is a header template (Inc/st7735.hpp, Inc/st7789.hpp):
# wiring constants (SPI handle, DC/CS/RST addresses and pins, sizes,
# orientation bits) are non-type template arguments, instantiating lives
# in the platform TU (main.cpp). There is nothing to compile here -
# the concrete object is created by main() and injected into Game.
add_library(${TARGET_LIB} INTERFACE)

target_include_directories(${TARGET_LIB} INTERFACE Inc)

# Driver headers include "main.h" (pin numbers, SPI_HandleTypeDef), so the
# platform HAL include path is offered to every consumer of the target.
# In a standalone Game_Core build the stm32cubemx target does not exist -
# the name degrades to a plain linker flag, which is harmless.
target_link_libraries(${TARGET_LIB} INTERFACE
    stm32cubemx)

# Reserved data note: there is no compiled C in this module. A future
# text HUD goes through sprite/digit assets (assets.cpp recipe) like any
# other panel data; if a raw C font archive is ever revived, declare a
# small STATIC target next to this interface - the INTERFACE target
# cannot host compiled sources.
