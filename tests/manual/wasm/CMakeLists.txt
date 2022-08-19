# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(eventloop)
add_subdirectory(rasterwindow)
add_subdirectory(a11y)
if(QT_FEATURE_widgets)
add_subdirectory(cursors)
add_subdirectory(localfiles)
add_subdirectory(qstdweb)
add_subdirectory(clipboard)
endif()
