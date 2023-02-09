# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(QT_FEATURE_cups AND UNIX AND NOT APPLE)
    add_subdirectory(cups)
endif()
