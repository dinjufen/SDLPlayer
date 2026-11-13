# Fallback finder used when vcpkg is not providing FFMPEG (for example, Linux
# system packages or Homebrew). It exposes the same variables as vcpkg's
# FindFFMPEG module.
#
# Official Qt kits ship FFmpeg dylibs on CMAKE_PREFIX_PATH for Qt Multimedia.
# Those are a different major version than Homebrew/system headers, and mixing
# them makes swr_init and similar calls fail at runtime. This module therefore
# never selects libraries from a Qt prefix.

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(PC_LIBAVFORMAT QUIET libavformat)
    pkg_check_modules(PC_LIBAVCODEC QUIET libavcodec)
    pkg_check_modules(PC_LIBAVUTIL QUIET libavutil)
    pkg_check_modules(PC_LIBSWRESAMPLE QUIET libswresample)
    pkg_check_modules(PC_LIBSWSCALE QUIET libswscale)
endif()

set(_ffmpeg_include_hints
    ${PC_LIBAVFORMAT_INCLUDEDIR}
    ${PC_LIBAVFORMAT_INCLUDE_DIRS}
    /opt/homebrew/include
    /opt/homebrew/opt/ffmpeg/include
    /usr/local/include
    /usr/local/opt/ffmpeg/include
    /usr/include
)
set(_ffmpeg_library_hints
    ${PC_LIBAVFORMAT_LIBRARY_DIRS}
    ${PC_LIBAVCODEC_LIBRARY_DIRS}
    ${PC_LIBAVUTIL_LIBRARY_DIRS}
    ${PC_LIBSWRESAMPLE_LIBRARY_DIRS}
    ${PC_LIBSWSCALE_LIBRARY_DIRS}
    /opt/homebrew/lib
    /opt/homebrew/opt/ffmpeg/lib
    /usr/local/lib
    /usr/local/opt/ffmpeg/lib
    /usr/lib
    /usr/lib/x86_64-linux-gnu
    /usr/lib/aarch64-linux-gnu
)
list(FILTER _ffmpeg_include_hints EXCLUDE REGEX "^$")
list(FILTER _ffmpeg_library_hints EXCLUDE REGEX "^$")
if(_ffmpeg_include_hints)
    list(REMOVE_DUPLICATES _ffmpeg_include_hints)
endif()
if(_ffmpeg_library_hints)
    list(REMOVE_DUPLICATES _ffmpeg_library_hints)
endif()

function(_ffmpeg_is_qt_bundled libpath out_var)
    if(libpath MATCHES "/[Qq]t/[0-9]")
        set(${out_var} TRUE PARENT_SCOPE)
    else()
        set(${out_var} FALSE PARENT_SCOPE)
    endif()
endfunction()

function(_ffmpeg_find_library varname libname)
    if(${varname})
        _ffmpeg_is_qt_bundled("${${varname}}" _is_qt)
        if(_is_qt)
            unset(${varname} CACHE)
            unset(${varname})
        else()
            return()
        endif()
    endif()

    find_library(${varname}
        NAMES ${libname}
        HINTS ${_ffmpeg_library_hints}
        PATHS ${_ffmpeg_library_hints}
        NO_DEFAULT_PATH
    )
    if(${varname})
        _ffmpeg_is_qt_bundled("${${varname}}" _is_qt)
        if(_is_qt)
            unset(${varname} CACHE)
            unset(${varname})
        endif()
    endif()

    if(NOT ${varname})
        find_library(${varname} NAMES ${libname})
        if(${varname})
            _ffmpeg_is_qt_bundled("${${varname}}" _is_qt)
            if(_is_qt)
                unset(${varname} CACHE)
                unset(${varname})
            endif()
        endif()
    endif()

    set(${varname} "${${varname}}" PARENT_SCOPE)
endfunction()

find_path(FFMPEG_INCLUDE_DIR
    NAMES libavformat/avformat.h
    HINTS ${_ffmpeg_include_hints}
    PATHS ${_ffmpeg_include_hints}
    NO_DEFAULT_PATH
)
if(NOT FFMPEG_INCLUDE_DIR)
    find_path(FFMPEG_INCLUDE_DIR NAMES libavformat/avformat.h)
endif()

_ffmpeg_find_library(FFMPEG_libavformat_LIBRARY avformat)
_ffmpeg_find_library(FFMPEG_libavcodec_LIBRARY avcodec)
_ffmpeg_find_library(FFMPEG_libavutil_LIBRARY avutil)
_ffmpeg_find_library(FFMPEG_libswresample_LIBRARY swresample)
_ffmpeg_find_library(FFMPEG_libswscale_LIBRARY swscale)

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
    message(STATUS "FFMPEG includes: ${FFMPEG_INCLUDE_DIR}")
    message(STATUS "FFMPEG libraries: ${FFMPEG_LIBRARIES}")
endif()

mark_as_advanced(
    FFMPEG_INCLUDE_DIR
    FFMPEG_libavformat_LIBRARY
    FFMPEG_libavcodec_LIBRARY
    FFMPEG_libavutil_LIBRARY
    FFMPEG_libswresample_LIBRARY
    FFMPEG_libswscale_LIBRARY
)
