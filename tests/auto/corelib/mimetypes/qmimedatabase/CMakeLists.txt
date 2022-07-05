# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qmimedatabase.pro.

if(TARGET Qt::Concurrent)
    add_subdirectory(qmimedatabase-xml)
endif()
if(TARGET Qt::Concurrent AND UNIX AND NOT APPLE AND NOT QNX)
    add_subdirectory(qmimedatabase-cache)
endif()
