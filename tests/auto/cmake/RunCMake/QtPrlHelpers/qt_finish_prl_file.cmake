# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

set(_helpers_dir "${QT_PRL_HELPERS_QTBASE_CMAKE_DIR}")
set(_in_file  "${CMAKE_CURRENT_BINARY_DIR}/in.prl")
set(_out_file "${CMAKE_CURRENT_BINARY_DIR}/out.prl")
set(_meta_file "${CMAKE_CURRENT_BINARY_DIR}/meta.txt")

# Synthetic step1 PRL: a -L from walk_libs followed by a bare library file
# name. After QtFinishPrlFile.cmake runs, the bare name must become
# "-lfoo" and the -L must pass through.
file(WRITE "${_in_file}" [=[
RCC_OBJECTS =
QMAKE_PRL_TARGET = libdummy.a
QMAKE_PRL_TARGET_PATH_FOR_CMAKE = /tmp/libdummy.a
QMAKE_PRL_CONFIG = static
QMAKE_PRL_VERSION = 6.12.0
QMAKE_PRL_LIBS_FOR_CMAKE = -L/abs/path;libfoo.so;-L
]=])

file(WRITE "${_meta_file}" "")

execute_process(
    COMMAND ${CMAKE_COMMAND}
        "-DIN_FILE=${_in_file}"
        "-DIN_META_FILE=${_meta_file}"
        "-DOUT_FILE=${_out_file}"
        "-DLIBRARY_PREFIXES=lib"
        "-DLIBRARY_SUFFIXES=.so;.a;.dylib;.lib;.dll"
        "-DLINK_LIBRARY_FLAG=-l"
        "-DQT_LIB_DIRS="
        "-DQT_PLUGIN_DIRS="
        "-DQT_QML_DIRS="
        "-DIMPLICIT_LINK_DIRECTORIES="
        -P "${_helpers_dir}/QtFinishPrlFile.cmake"
    RESULT_VARIABLE _rc
    OUTPUT_VARIABLE _stdout
    ERROR_VARIABLE  _stderr)

if(NOT _rc EQUAL 0)
    message(FATAL_ERROR
        "QtFinishPrlFile.cmake failed (rc=${_rc})\n"
        "stdout: ${_stdout}\n"
        "stderr: ${_stderr}")
endif()

file(READ "${_out_file}" _out)

if(NOT _out MATCHES "QMAKE_PRL_LIBS = .*-L/abs/path.*-lfoo")
    message(FATAL_ERROR
        "Output PRL did not contain '-L/abs/path' followed by '-lfoo'.\n"
        "Output:\n${_out}")
endif()

if(_out MATCHES "QMAKE_PRL_LIBS = .*[^-]libfoo\\.so")
    message(FATAL_ERROR
        "Output PRL still contains a bare 'libfoo.so' literal.\n"
        "Output:\n${_out}")
endif()

if(_out MATCHES "QMAKE_PRL_LIBS_FOR_CMAKE = ([^\n]+)")
    if(CMAKE_MATCH_1 MATCHES "(^|;)-L(;|$)")
        message(FATAL_ERROR
            "Output PRL contains a stray '-L' token (no path).\n"
            "Output:\n${_out}")
    endif()
else()
    message(FATAL_ERROR "Output PRL did not contain QMAKE_PRL_LIBS_FOR_CMAKE line.\n${_out}")
endif()
