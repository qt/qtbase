# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(qstandarditem)
if(TARGET Qt::Widgets)
    add_subdirectory(qstandarditemmodel)
endif()
if(TARGET Qt::Widgets AND NOT MINGW)
    add_subdirectory(qfilesystemmodel)
endif()
