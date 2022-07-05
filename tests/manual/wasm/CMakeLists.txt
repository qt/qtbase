# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

add_subdirectory(eventloop)
add_subdirectory(rasterwindow)
add_subdirectory(a11y)
if(QT_FEATURE_widgets)
add_subdirectory(cursors)
add_subdirectory(localfiles)
add_subdirectory(qstdweb)
add_subdirectory(clipboard)
endif()
