cmake_minimum_required(VERSION 3.20)
project(ff1_cpp CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(FF1_ENABLE_TESTS "Build verification unit tests" ON)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Include FetchContent for SDL2
include(FetchContent)
FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-2.30.1
    GIT_SHALLOW    TRUE
)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL2)

# Source files for core data & game engine library
set(FF1_CORE_SOURCES
    src/data/data_loader.cpp
    src/data/map_loader.cpp
    src/data/text_decoder.cpp
    src/engine/rng.cpp
    src/state/save_system.cpp
    src/core/battle_engine.cpp
    src/core/map_engine.cpp
    src/core/menu_engine.cpp
    src/core/intro_engine.cpp
    src/ui/window_box.cpp
    src/ui/font.cpp
    src/engine/renderer.cpp
    src/engine/system.cpp
    src/engine/audio_engine.cpp
    src/engine/mod_loader.cpp
    src/engine/chr_decoder.cpp
    src/core/cutscene_engine.cpp
    src/core/minigame_engine.cpp
)

# Core engine static library
add_library(ff1_core STATIC ${FF1_CORE_SOURCES})
target_link_libraries(ff1_core PUBLIC SDL2::SDL2main SDL2::SDL2-static)

# Main executable
add_executable(ff1_cpp src/main.cpp)
target_link_libraries(ff1_cpp PRIVATE ff1_core SDL2::SDL2main SDL2::SDL2-static)

# Unit tests executable
if(FF1_ENABLE_TESTS)
    add_executable(test_loader tests/test_loader.cpp)
    target_link_libraries(test_loader PRIVATE ff1_core SDL2::SDL2main SDL2::SDL2-static)
endif()
