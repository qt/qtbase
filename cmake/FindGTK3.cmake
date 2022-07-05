# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

find_package(PkgConfig QUIET)

set(__gtk3_required_version "${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION}")
if(__gtk3_required_version)
    set(__gtk3_required_version " >= ${__gtk3_required_version}")
endif()
pkg_check_modules(GTK3 "gtk+-3.0${__gtk3_required_version}" IMPORTED_TARGET)

if (NOT TARGET PkgConfig::GTK3)
    set(GTK3_FOUND 0)
endif()
unset(__gtk3_required_version)
