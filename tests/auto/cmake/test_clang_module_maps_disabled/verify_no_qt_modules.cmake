# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

file(GLOB_RECURSE qt_modules "${modules_cache_path}/Qt*.pcm")
if(qt_modules)
    list(JOIN qt_modules "\n  " qt_modules)
    message(FATAL_ERROR
        "Qt Clang modules were built even though Qt's module maps should have been "
        "masked by a VFS overlay:\n  ${qt_modules}")
endif()
