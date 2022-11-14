# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT TARGET Qt6::DBus)
    return()
endif()
qt_internal_add_example(listnames)
qt_internal_add_example(pingpong)
if(QT_FEATURE_process)
    qt_internal_add_example(complexpingpong)
endif()
if(TARGET Qt6::Widgets)
    qt_internal_add_example(chat)
    add_subdirectory(remotecontrolledcar)
endif()
