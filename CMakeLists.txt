cmake_minimum_required(VERSION 3.20)
project(legacy_c_rpg C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(rpg_core src/game.c)
target_include_directories(rpg_core PUBLIC include)

target_compile_options(rpg_core PRIVATE -Wall -Wextra -Wpedantic)

add_executable(rpg_tests tests/test_game.c)
target_link_libraries(rpg_tests PRIVATE rpg_core)

enable_testing()
add_test(NAME rpg_core_tests COMMAND rpg_tests)
