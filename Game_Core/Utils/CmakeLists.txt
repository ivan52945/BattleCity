cmake_minimum_required(VERSION 3.22)

SET(SRC Src)
SET(INC Inc)

SET(TARGET_LIB utils)

add_library(${TARGET_LIB} INTERFACE)

# target_sources(${TARGET_LIB} PRIVATE)

target_include_directories(${TARGET_LIB} INTERFACE ${INC})

