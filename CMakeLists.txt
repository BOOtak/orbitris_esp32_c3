cmake_minimum_required(VERSION 3.25)
project(orbitris_esp32 C CXX)

set(CMAKE_CXX_STANDARD 17)

# Setting parameters for raylib
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE) # or games

if (NOT MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-multichar")
endif()

add_subdirectory(lib/raylib)

add_executable(${PROJECT_NAME}
    raylib_adapter/main.cpp
    raylib_adapter/input_raylib.cpp
    raylib_adapter/lcd_raylib.cpp
    raylib_adapter/trace_printf.cpp

    orbitris_esp32/button.cpp
    orbitris_esp32/button_grid_manager.cpp
    orbitris_esp32/draw.cpp
    orbitris_esp32/explosion.cpp
    orbitris_esp32/game_main.cpp
    orbitris_esp32/game_over_screen.cpp
    orbitris_esp32/game_screen.cpp
    orbitris_esp32/game_utils.cpp
    orbitris_esp32/orbital.cpp
    orbitris_esp32/menu_screen.cpp
    orbitris_esp32/pause_screen.cpp
    orbitris_esp32/screen.cpp
    orbitris_esp32/table_math.cpp
    orbitris_esp32/tetramino.cpp
    orbitris_esp32/tilemap.cpp
    orbitris_esp32/transition.cpp
    )

target_link_libraries(${PROJECT_NAME} PRIVATE raylib)