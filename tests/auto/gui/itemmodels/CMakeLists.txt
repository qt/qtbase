# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from itemmodels.pro.

add_subdirectory(qstandarditem)
if(TARGET Qt::Widgets)
    add_subdirectory(qstandarditemmodel)
endif()
if(TARGET Qt::Widgets AND NOT MINGW)
    add_subdirectory(qfilesystemmodel)
endif()
