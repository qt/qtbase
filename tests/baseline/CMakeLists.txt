# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

if(TARGET Qt::Network)
    add_subdirectory(painting)
endif()
if(TARGET Qt::Network AND TARGET Qt::Widgets)
    add_subdirectory(widgets)
    add_subdirectory(stylesheet)
    add_subdirectory(text)
endif()
