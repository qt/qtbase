# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

add_subdirectory(ipc)
add_subdirectory(mimetypes)
add_subdirectory(serialization)
add_subdirectory(tools)
add_subdirectory(platform)
add_subdirectory(time)
if(QT_FEATURE_thread)
    add_subdirectory(threads)
endif()
if(QT_FEATURE_widgets)
    qt_internal_add_example(bindableproperties)
endif()
