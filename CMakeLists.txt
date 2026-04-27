cmake_minimum_required(VERSION 3.25)

project(SDLPlayer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_ttf CONFIG REQUIRED)
find_package(FFMPEG REQUIRED)

enable_testing()

add_library(SDLPlayerCore STATIC
    src/player/Player.cpp
    src/player/Player.h
)

target_compile_definitions(SDLPlayerCore PRIVATE
    _CRT_SECURE_NO_WARNINGS
)

if(MSVC)
    target_compile_options(SDLPlayerCore PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(SDLPlayerCore PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_include_directories(SDLPlayerCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${FFMPEG_INCLUDE_DIRS}
)

target_link_directories(SDLPlayerCore PUBLIC
    ${FFMPEG_LIBRARY_DIRS}
)

target_link_libraries(SDLPlayerCore PUBLIC
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
    ${FFMPEG_libavformat_LIBRARY}
    ${FFMPEG_libavcodec_LIBRARY}
    ${FFMPEG_libavutil_LIBRARY}
    ${FFMPEG_libswresample_LIBRARY}
    ${FFMPEG_libswscale_LIBRARY}
)

add_executable(SDLPlayer WIN32
    src/main.cpp
)

target_compile_definitions(SDLPlayer PRIVATE
    _CRT_SECURE_NO_WARNINGS
)

if(MSVC)
    target_compile_options(SDLPlayer PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(SDLPlayer PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_link_libraries(SDLPlayer PRIVATE
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2_ttf::SDL2_ttf>,SDL2_ttf::SDL2_ttf,SDL2_ttf::SDL2_ttf-static>
    SDLPlayerCore
    comdlg32
)

set_target_properties(SDLPlayer PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

add_custom_command(TARGET SDLPlayer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_RUNTIME_DLLS:SDLPlayer>
        $<TARGET_FILE_DIR:SDLPlayer>
    COMMAND_EXPAND_LISTS
)

add_executable(SDLPlayerTests
    tests/player/PlayerTests.cpp
)

target_compile_definitions(SDLPlayerTests PRIVATE
    _CRT_SECURE_NO_WARNINGS
)

if(MSVC)
    target_compile_options(SDLPlayerTests PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(SDLPlayerTests PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_link_libraries(SDLPlayerTests PRIVATE
    SDLPlayerCore
)

add_test(NAME SDLPlayerTests COMMAND SDLPlayerTests)

set_tests_properties(SDLPlayerTests PROPERTIES
    WORKING_DIRECTORY $<TARGET_FILE_DIR:SDLPlayerTests>
)

add_custom_command(TARGET SDLPlayerTests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_RUNTIME_DLLS:SDLPlayerTests>
        $<TARGET_FILE_DIR:SDLPlayerTests>
    COMMAND_EXPAND_LISTS
)
