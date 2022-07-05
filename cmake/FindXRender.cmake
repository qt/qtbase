# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

find_package(PkgConfig QUIET)

if(NOT TARGET PkgConfig::XRender)
    pkg_check_modules(XRender xrender IMPORTED_TARGET)

    if (NOT TARGET PkgConfig::XRender)
        set(XRender_FOUND 0)
    endif()
else()
    set(XRender_FOUND 1)
endif()
