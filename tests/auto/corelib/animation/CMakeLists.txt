# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from animation.pro.

add_subdirectory(qabstractanimation)
add_subdirectory(qanimationgroup)
add_subdirectory(qparallelanimationgroup)
add_subdirectory(qpauseanimation)
add_subdirectory(qsequentialanimationgroup)
add_subdirectory(qvariantanimation)
if(TARGET Qt::Widgets)
    add_subdirectory(qpropertyanimation)
endif()
