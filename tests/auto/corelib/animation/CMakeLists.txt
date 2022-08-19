# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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
