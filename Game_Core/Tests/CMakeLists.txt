cmake_minimum_required(VERSION 3.20)
project(TanksTests CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# AddressSanitizer + UBSan for the boundary cases (e.g. the ">= -> >"
# mutant of the pool reads items[N]): the tests catch that mutant
# behaviorally (garbage non-null instead of nullptr), the sanitizer
# adds a hard UB report. mingw-w64 gcc ships NO ASan/UBSan runtimes
# (no libasan on Windows) - configure with -D TESTS_SANITIZERS=OFF
# there; enable it on toolchains that have libasan (linux clang/gcc).
option(TESTS_SANITIZERS "ASan + UBSan for the test build" OFF)

set(CORE ${CMAKE_CURRENT_SOURCE_DIR}/..)

# Host test target: the sprite asset unit is linked directly (the
# display target pulls HAL - not usable on the host).
function(core_test name)
    add_executable(${name} ${name}.cpp ${CORE}/Game/Src/assets.cpp)

    target_include_directories(${name} PRIVATE
        ${CORE}/Game/Inc
        ${CORE}/Utils/Inc
        ${CORE}/DeviceDrivers/Display/Inc
        ${CMAKE_CURRENT_SOURCE_DIR}/doctest)

    target_compile_options(${name} PRIVATE -Wall -Wextra)

    # Same trap as Platforms/Host: a foreign libstdc++-6.dll in PATH
    # hits STATUS_ENTRYPOINT_NOT_FOUND - link the binary statically.
    if(WIN32)
        target_link_options(${name} PRIVATE -static)
    endif()

    if(TESTS_SANITIZERS)
        target_compile_options(${name} PRIVATE -fsanitize=address,undefined)
        target_link_options(${name} PRIVATE -fsanitize=address,undefined)
    endif()

    add_test(NAME ${name} COMMAND ${name})

    # Tests run right after the build: a failing case fails the build.
    # ctest stays for targeted runs and the future CI.
    add_custom_command(TARGET ${name} POST_BUILD
        COMMAND $<TARGET_FILE:${name}>)
endfunction()

core_test(test_lobby)  # the Lobby<T, N> pool suite
core_test(test_map)    # the MapT<LcdT> terrain suite
