cmake_minimum_required(VERSION 3.22)

SET(SRC Src)
SET(INC Inc)

SET(GAME_CORE game_core)

add_library(${GAME_CORE})

target_sources(${GAME_CORE} PRIVATE
    ${SRC}/assets.cpp
    ${SRC}/entity.cpp
    ${SRC}/tank.cpp
    ${SRC}/shell.cpp
    ${SRC}/game.cpp)

target_include_directories(${GAME_CORE} PUBLIC ${INC})

target_link_libraries(${GAME_CORE} PRIVATE
    utils
    display)