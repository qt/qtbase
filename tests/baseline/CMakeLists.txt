# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(TARGET Qt::Gui AND TARGET Qt::Network AND QT_FEATURE_pdf)
    add_subdirectory(painting)
endif()
if(TARGET Qt::Network AND TARGET Qt::Widgets)
    add_subdirectory(widgets)
    add_subdirectory(stylesheet)
    add_subdirectory(text)
endif()
