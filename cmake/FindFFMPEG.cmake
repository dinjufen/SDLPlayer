# Fallback finder used when vcpkg is not providing FFMPEG (for example, Linux
# system packages). It exposes the same variables as vcpkg's FindFFMPEG module.

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(PC_LIBAVFORMAT QUIET libavformat)
    pkg_check_modules(PC_LIBAVCODEC QUIET libavcodec)
    pkg_check_modules(PC_LIBAVUTIL QUIET libavutil)
    pkg_check_modules(PC_LIBSWRESAMPLE QUIET libswresample)
    pkg_check_modules(PC_LIBSWSCALE QUIET libswscale)
endif()

find_path(FFMPEG_INCLUDE_DIR
    NAMES libavformat/avformat.h
    HINTS ${PC_LIBAVFORMAT_INCLUDEDIR} ${PC_LIBAVFORMAT_INCLUDE_DIRS}
)

find_library(FFMPEG_libavformat_LIBRARY NAMES avformat HINTS ${PC_LIBAVFORMAT_LIBRARY_DIRS})
find_library(FFMPEG_libavcodec_LIBRARY NAMES avcodec HINTS ${PC_LIBAVCODEC_LIBRARY_DIRS})
find_library(FFMPEG_libavutil_LIBRARY NAMES avutil HINTS ${PC_LIBAVUTIL_LIBRARY_DIRS})
find_library(FFMPEG_libswresample_LIBRARY NAMES swresample HINTS ${PC_LIBSWRESAMPLE_LIBRARY_DIRS})
find_library(FFMPEG_libswscale_LIBRARY NAMES swscale HINTS ${PC_LIBSWSCALE_LIBRARY_DIRS})

if(FFMPEG_libavformat_LIBRARY)
    get_filename_component(FFMPEG_LIBRARY_DIRS "${FFMPEG_libavformat_LIBRARY}" DIRECTORY)
endif()

set(FFMPEG_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIR})

find_package_handle_standard_args(FFMPEG
    REQUIRED_VARS
        FFMPEG_INCLUDE_DIR
        FFMPEG_libavformat_LIBRARY
        FFMPEG_libavcodec_LIBRARY
        FFMPEG_libavutil_LIBRARY
        FFMPEG_libswresample_LIBRARY
        FFMPEG_libswscale_LIBRARY
)

if(FFMPEG_FOUND)
    set(FFMPEG_LIBRARIES
        ${FFMPEG_libavformat_LIBRARY}
        ${FFMPEG_libavcodec_LIBRARY}
        ${FFMPEG_libavutil_LIBRARY}
        ${FFMPEG_libswresample_LIBRARY}
        ${FFMPEG_libswscale_LIBRARY}
    )
endif()

mark_as_advanced(
    FFMPEG_INCLUDE_DIR
    FFMPEG_libavformat_LIBRARY
    FFMPEG_libavcodec_LIBRARY
    FFMPEG_libavutil_LIBRARY
    FFMPEG_libswresample_LIBRARY
    FFMPEG_libswscale_LIBRARY
)
