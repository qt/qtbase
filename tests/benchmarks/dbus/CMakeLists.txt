# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(qdbustype)
if(QT_FEATURE_process)
    add_subdirectory(qdbusperformance)
endif()
