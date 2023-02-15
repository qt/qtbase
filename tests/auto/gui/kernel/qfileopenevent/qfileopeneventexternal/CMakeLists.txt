# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qfileopeneventexternal Binary:
#####################################################################

qt_internal_add_executable(qfileopeneventexternal
    GUI
    SOURCES
        qfileopeneventexternal.cpp
    LIBRARIES
        Qt::Gui
)
