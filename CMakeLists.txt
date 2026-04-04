cmake_minimum_required(VERSION 3.25)

project(SDLPlayer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_ttf CONFIG REQUIRED)
find_package(FFMPEG REQUIRED)

add_executable(SDLPlayer WIN32
    src/main.cpp
    src/player/Player.cpp
    src/player/Player.h
)

target_compile_definitions(SDLPlayer PRIVATE
    _CRT_SECURE_NO_WARNINGS
)

if(MSVC)
    target_compile_options(SDLPlayer PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(SDLPlayer PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_include_directories(SDLPlayer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${FFMPEG_INCLUDE_DIRS}
)

target_link_directories(SDLPlayer PRIVATE
    ${FFMPEG_LIBRARY_DIRS}
)

target_link_libraries(SDLPlayer PRIVATE
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
    $<IF:$<TARGET_EXISTS:SDL2_ttf::SDL2_ttf>,SDL2_ttf::SDL2_ttf,SDL2_ttf::SDL2_ttf-static>
    ${FFMPEG_LIBRARIES}
    comdlg32
)

set_target_properties(SDLPlayer PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
