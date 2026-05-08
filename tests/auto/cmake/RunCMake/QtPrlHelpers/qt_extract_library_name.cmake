# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

include("${QT_PRL_HELPERS_QTBASE_CMAKE_DIR}/QtGenerateLibHelpers.cmake")

# Mimic what QtPrlHelpers.cmake supplies when invoking QtFinishPrlFile.
set(LIBRARY_PREFIXES "lib")
set(LIBRARY_SUFFIXES ".so" ".a" ".dylib" ".lib" ".dll")

function(check_extract input expected)
    qt_extract_library_name(actual "${input}")
    if(NOT actual STREQUAL expected)
        message(FATAL_ERROR
            "qt_extract_library_name(\"${input}\") returned \"${actual}\","
            " expected \"${expected}\"")
    endif()
endfunction()

check_extract("libavformat.so"               "avformat")
check_extract("libavformat.so.60.16.100"     "avformat")
check_extract("/p/libfoo.a"                  "foo")
check_extract("foo.lib"                      "foo")
check_extract("libfoo.dylib"                 "foo")
check_extract("Threads::Threads"             "NOTFOUND")
check_extract("-lfoo"                        "NOTFOUND")
check_extract(""                             "NOTFOUND")
