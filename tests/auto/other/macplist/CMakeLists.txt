# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT TARGET Qt::Widgets)
    return()
endif()
add_subdirectory(app)
add_subdirectory(test)
