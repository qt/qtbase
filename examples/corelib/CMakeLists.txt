# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

add_subdirectory(bindableproperties)
add_subdirectory(ipc)
add_subdirectory(mimetypes)
add_subdirectory(serialization)
add_subdirectory(tools)
add_subdirectory(platform)
if(QT_FEATURE_thread)
    add_subdirectory(threads)
endif()
