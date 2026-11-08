cmake_minimum_required(VERSION 3.22)

SET(SRC Src)
SET(INC Inc)

SET(GAME_CORE game_core)

# All gameplay code (Entity/Tank/Shell/GameT) lives in the headers as
# templates over the concrete driver type - assets.cpp keeps the
# flash-resident sprite data and is the only translation unit left.
add_library(${GAME_CORE})

target_sources(${GAME_CORE} PRIVATE
    ${SRC}/assets.cpp)

target_include_directories(${GAME_CORE} PUBLIC ${INC})

target_link_libraries(${GAME_CORE} PUBLIC
    utils)

target_link_libraries(${GAME_CORE} PRIVATE
    display)
