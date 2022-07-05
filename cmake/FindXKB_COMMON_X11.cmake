# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

find_package(PkgConfig QUIET)

pkg_check_modules(XKB_COMMON_X11 "xkbcommon-x11>=0.4.1" IMPORTED_TARGET)

if (NOT TARGET PkgConfig::XKB_COMMON_X11)
    set(XKB_COMMON_X11_FOUND 0)
endif()
