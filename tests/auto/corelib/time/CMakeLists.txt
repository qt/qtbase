# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from time.pro.

add_subdirectory(qcalendar)
add_subdirectory(qdate)
add_subdirectory(qdatetime)
add_subdirectory(qdatetimeparser)
add_subdirectory(qtime)
if(QT_FEATURE_timezone AND NOT INTEGRITY)
    add_subdirectory(qtimezone)
endif()
