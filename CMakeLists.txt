cmake_minimum_required(VERSION 3.20)
project(legacy_c_rpg VERSION 2.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(RPG_ENABLE_SANITIZERS "Enable Address/UndefinedBehavior sanitizers" OFF)

add_library(rpg_core
    src/game.c
    src/combat.c
    src/shop.c
    src/save.c
)
target_include_directories(rpg_core PUBLIC include)
target_compile_options(rpg_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)

if(RPG_ENABLE_SANITIZERS AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(rpg_core PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(rpg_core PRIVATE -fsanitize=address,undefined)
endif()

add_executable(legacy_rpg src/main.c)
target_link_libraries(legacy_rpg PRIVATE rpg_core)
target_compile_options(legacy_rpg PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)

add_executable(rpg_tests tests/test_game.c)
target_link_libraries(rpg_tests PRIVATE rpg_core)
target_compile_options(rpg_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)

if(RPG_ENABLE_SANITIZERS AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(rpg_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(rpg_tests PRIVATE -fsanitize=address,undefined)
endif()

enable_testing()
add_test(NAME rpg_regression_tests COMMAND rpg_tests)
