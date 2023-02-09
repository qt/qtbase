# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


if(QT_FEATURE_dbus AND QT_FEATURE_mimetype AND QT_FEATURE_regularexpression AND UNIX AND NOT APPLE)
    add_subdirectory(xdgdesktopportal)
endif()
if(QT_FEATURE_gtk3 AND TARGET Qt::Widgets)
    add_subdirectory(gtk3)
endif()
