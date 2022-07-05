# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

if(NOT TARGET Qt::DBus)
    return()
endif()
qt_internal_add_example(listnames)
qt_internal_add_example(pingpong)
if(QT_FEATURE_process)
    qt_internal_add_example(complexpingpong)
endif()
if(TARGET Qt::Widgets)
    qt_internal_add_example(chat)
    add_subdirectory(remotecontrolledcar)
endif()
