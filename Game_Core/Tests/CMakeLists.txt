cmake_minimum_required(VERSION 3.20)
project(TanksTests CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# AddressSanitizer + UBSan for the "activate_on_full" boundary case
# (the ">= -> >" mutant reads items[N]): the test catches that mutant
# behaviorally (garbage non-null instead of nullptr), the sanitizer
# adds a hard UB report. mingw-w64 gcc ships NO ASan/UBSan runtimes
# (no libasan on Windows) - configure with -D TESTS_SANITIZERS=OFF
# there; enable it on toolchains that have libasan (linux clang/gcc).
option(TESTS_SANITIZERS "ASan + UBSan for the test build" OFF)

add_executable(test_lobby test_lobby.cpp)

target_include_directories(test_lobby PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../Utils/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/doctest)

target_compile_options(test_lobby PRIVATE -Wall -Wextra)

# Same trap as Platforms/Host: a foreign libstdc++-6.dll in PATH hits
# STATUS_ENTRYPOINT_NOT_FOUND - link the test binary statically.
if(WIN32)
    target_link_options(test_lobby PRIVATE -static)
endif()

if(TESTS_SANITIZERS)
    target_compile_options(test_lobby PRIVATE -fsanitize=address,undefined)
    target_link_options(test_lobby PRIVATE -fsanitize=address,undefined)
endif()

enable_testing()
add_test(NAME lobby COMMAND test_lobby)

# Tests run right after the build: a failing case fails the build.
# ctest stays for targeted runs and the future CI.
add_custom_command(TARGET test_lobby POST_BUILD
    COMMAND $<TARGET_FILE:test_lobby>)
