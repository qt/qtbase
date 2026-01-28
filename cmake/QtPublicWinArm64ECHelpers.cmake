# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

function(_qt_internal_set_win_arm64ec_env_flags)
    if("${QT_QMAKE_TARGET_MKSPEC}" STREQUAL "win32-arm64ec-msvc")
        if(NOT DEFINED ENV{CFLAGS})
            set(ENV{CFLAGS} "-arm64EC")
        elseif(NOT $ENV{CFLAGS} MATCHES "[-/]arm64EC")
            set(ENV{CFLAGS} "$ENV{CFLAGS} -arm64EC")
        endif()

        if(NOT DEFINED ENV{CXXFLAGS})
            set(ENV{CXXFLAGS} "-arm64EC")
        elseif(NOT $ENV{CXXFLAGS} MATCHES "[-/]arm64EC")
            set(ENV{CXXFLAGS} "$ENV{CXXFLAGS} -arm64EC")
        endif()

        if(NOT $ENV{CXXFLAGS} MATCHES "[-/]d2arm64ECMarkAllFuncsPatchable")
            set(ENV{CXXFLAGS} "$ENV{CXXFLAGS} -d2arm64ECMarkAllFuncsPatchable")
        endif()
    endif()
endfunction()
