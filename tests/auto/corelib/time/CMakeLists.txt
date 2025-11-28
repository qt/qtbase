# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(qcalendar)
add_subdirectory(qdate)
add_subdirectory(qdatetime)
if(QT_FEATURE_datetimeparser)
    add_subdirectory(qdatetimeparser)
endif()
if(QT_FEATURE_datestring)
    add_subdirectory(qdatetimepattern)
    add_subdirectory(qtimepattern)
    add_subdirectory(qdatepattern)
endif()
add_subdirectory(qtime)
add_subdirectory(qtimezone)
if(QT_FEATURE_timezone AND QT_FEATURE_developer_build)
    add_subdirectory(qtimezonebackend)
endif()
