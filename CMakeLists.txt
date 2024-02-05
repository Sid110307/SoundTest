cmake_minimum_required(VERSION 3.20)
project(SoundTest)

set(CMAKE_CXX_STANDARD 20)

if (MSVC)
    set(CMAKE_CXX_FLAGS "/W4 /WX")
else ()
    set(CMAKE_CXX_FLAGS "-Wall -Wextra")
endif ()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(soundTest ${PROJECT_SOURCE_DIR}/main.cpp)

target_sources(soundTest PUBLIC
        lib/imgui/imgui.cpp
        lib/imgui/imgui_draw.cpp
        lib/imgui/imgui_tables.cpp
        lib/imgui/imgui_widgets.cpp
        lib/imgui/imgui_impl_opengl3.cpp
        lib/imgui/imgui_impl_sdl2.cpp
)

find_package(SDL2 REQUIRED)

target_link_libraries(soundTest PUBLIC ${CMAKE_DL_LIBS} pthread ${SDL2_LIBRARIES})
target_include_directories(soundTest PUBLIC lib ${SDL2_INCLUDE_DIRS})
