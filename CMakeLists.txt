cmake_minimum_required(VERSION 3.25)

project(SDLPlayer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SDLPLAYER_BUILD_SDL "Build the legacy SDL2 shell (SDLPlayerSdl)" ON)
option(SDLPLAYER_BUILD_QT "Build the Qt 6 Quick shell (SDLPlayer)" ON)
option(SDLPLAYER_REQUIRE_QT "Fail configure if Qt 6.9+ is not found" OFF)
option(SDLPLAYER_REQUIRE_SDL "Fail configure if SDL2 is not found" OFF)

find_package(FFMPEG QUIET)
if(NOT FFMPEG_FOUND)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
    find_package(FFMPEG REQUIRED)
endif()

find_package(Threads REQUIRED)

enable_testing()

add_library(SDLPlayerCore STATIC
    src/player/Player.cpp
    src/player/Player.h
    src/player/IAudioOutput.h
    src/player/IVideoOutput.h
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
    ${FFMPEG_libavformat_LIBRARY}
    ${FFMPEG_libavcodec_LIBRARY}
    ${FFMPEG_libavutil_LIBRARY}
    ${FFMPEG_libswresample_LIBRARY}
    ${FFMPEG_libswscale_LIBRARY}
    Threads::Threads
)

# ---------------------------------------------------------------------------
# Legacy SDL shell
# ---------------------------------------------------------------------------
if(SDLPLAYER_BUILD_SDL)
    find_package(SDL2 CONFIG QUIET)
    find_package(SDL2_ttf CONFIG QUIET)
    if(NOT SDL2_FOUND OR NOT SDL2_ttf_FOUND)
        if(SDLPLAYER_REQUIRE_SDL)
            find_package(SDL2 CONFIG REQUIRED)
            find_package(SDL2_ttf CONFIG REQUIRED)
        else()
            message(WARNING "SDL2 or SDL2_ttf was not found; skipping the legacy SDLPlayerSdl target.")
            set(SDLPLAYER_BUILD_SDL OFF)
        endif()
    endif()
endif()

if(SDLPLAYER_BUILD_SDL)
    set(SDLPlayerSdl_SOURCES
        src/main.cpp
        src/output/SdlAudioOutput.cpp
        src/output/SdlAudioOutput.h
        src/output/SdlVideoOutput.cpp
        src/output/SdlVideoOutput.h
        src/ui/SdlHiDpi.cpp
        src/ui/SdlHiDpi.h
        src/platform/Platform.h
    )

    if(WIN32)
        list(APPEND SDLPlayerSdl_SOURCES src/platform/PlatformWindows.cpp)
        add_executable(SDLPlayerSdl WIN32 ${SDLPlayerSdl_SOURCES})
        target_link_libraries(SDLPlayerSdl PRIVATE comdlg32)
    elseif(APPLE)
        enable_language(OBJCXX)
        list(APPEND SDLPlayerSdl_SOURCES src/platform/PlatformMac.mm)
        add_executable(SDLPlayerSdl ${SDLPlayerSdl_SOURCES})
        set_source_files_properties(src/platform/PlatformMac.mm PROPERTIES
            COMPILE_FLAGS "-fobjc-arc"
        )
        target_link_libraries(SDLPlayerSdl PRIVATE "-framework AppKit" "-framework UniformTypeIdentifiers")
        set_target_properties(SDLPlayerSdl PROPERTIES
            MACOSX_RPATH ON
            BUILD_WITH_INSTALL_RPATH ON
            BUILD_RPATH "@executable_path;@loader_path"
            INSTALL_RPATH "@executable_path;@loader_path"
        )
    else()
        list(APPEND SDLPlayerSdl_SOURCES src/platform/PlatformLinux.cpp)
        add_executable(SDLPlayerSdl ${SDLPlayerSdl_SOURCES})
    endif()

    target_compile_definitions(SDLPlayerSdl PRIVATE
        _CRT_SECURE_NO_WARNINGS
    )

    if(MSVC)
        target_compile_options(SDLPlayerSdl PRIVATE /W4 /permissive- /utf-8)
    else()
        target_compile_options(SDLPlayerSdl PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    target_link_libraries(SDLPlayerSdl PRIVATE
        $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
        $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
        $<IF:$<TARGET_EXISTS:SDL2_ttf::SDL2_ttf>,SDL2_ttf::SDL2_ttf,SDL2_ttf::SDL2_ttf-static>
        SDLPlayerCore
    )

    set_target_properties(SDLPlayerSdl PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )

    if(WIN32)
        add_custom_command(TARGET SDLPlayerSdl POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:SDLPlayerSdl>
                $<TARGET_FILE_DIR:SDLPlayerSdl>
            COMMAND_EXPAND_LISTS
        )
    endif()
endif()

# ---------------------------------------------------------------------------
# Primary Qt 6 Quick / QML shell
# ---------------------------------------------------------------------------
if(SDLPLAYER_BUILD_QT)
    if(SDLPLAYER_REQUIRE_QT)
        find_package(Qt6 6.9 REQUIRED COMPONENTS Quick QuickControls2 QuickDialogs2 Multimedia)
    else()
        find_package(Qt6 6.9 QUIET COMPONENTS Quick QuickControls2 QuickDialogs2 Multimedia)
        if(NOT Qt6_FOUND)
            message(WARNING "Qt 6.9+ was not found; skipping the SDLPlayer Qt shell. Set CMAKE_PREFIX_PATH to a Qt 6.9 install, or use the macos-qt-debug preset.")
            set(SDLPLAYER_BUILD_QT OFF)
        endif()
    endif()
endif()

if(SDLPLAYER_BUILD_QT)
    qt_standard_project_setup(REQUIRES 6.9)

    qt_add_executable(SDLPlayer
        src/qt/main.cpp
        src/qt/PlayerController.cpp
        src/qt/PlayerController.h
        src/qt/QtAudioOutput.cpp
        src/qt/QtAudioOutput.h
        src/qt/QtVideoOutput.cpp
        src/qt/QtVideoOutput.h
    )

    set_source_files_properties(src/qt/qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

    qt_add_qml_module(SDLPlayer
        URI SDLPlayer
        VERSION 1.0
        QML_FILES
            src/qt/qml/Main.qml
            src/qt/qml/Theme.qml
            src/qt/qml/ControlBar.qml
            src/qt/qml/SeekBar.qml
            src/qt/qml/SpeedMenu.qml
            src/qt/qml/VolumeControl.qml
            src/qt/qml/IconButton.qml
            src/qt/qml/EmptyState.qml
            src/qt/qml/AudioPlaceholder.qml
            src/qt/qml/MediaInfoOverlay.qml
        SOURCES
            src/qt/VideoItem.cpp
            src/qt/VideoItem.h
    )

    if(MSVC)
        target_compile_options(SDLPlayer PRIVATE /W4 /permissive- /utf-8)
    else()
        target_compile_options(SDLPlayer PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    target_link_libraries(SDLPlayer PRIVATE
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::QuickDialogs2
        Qt6::Multimedia
        SDLPlayerCore
    )

    set_target_properties(SDLPlayer PROPERTIES
        WIN32_EXECUTABLE TRUE
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )

    if(APPLE)
        set_property(TARGET SDLPlayer APPEND PROPERTY BUILD_RPATH
            "${FFMPEG_LIBRARY_DIRS}"
            /opt/homebrew/lib
        )
        set_property(TARGET SDLPlayer APPEND PROPERTY INSTALL_RPATH
            "@executable_path"
            "@loader_path"
        )
    endif()

    if(WIN32)
        add_custom_command(TARGET SDLPlayer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:SDLPlayer>
                $<TARGET_FILE_DIR:SDLPlayer>
            COMMAND_EXPAND_LISTS
        )
    endif()
endif()

# ---------------------------------------------------------------------------
# Tests (player core only)
# ---------------------------------------------------------------------------
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

if(WIN32)
    add_custom_command(TARGET SDLPlayerTests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:SDLPlayerTests>
            $<TARGET_FILE_DIR:SDLPlayerTests>
        COMMAND_EXPAND_LISTS
    )
endif()
