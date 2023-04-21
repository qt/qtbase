# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(qsignalspy)
add_subdirectory(tostring)

# QTBUG-88507
if(QT_FEATURE_process AND NOT ANDROID)
    add_subdirectory(selftests)
endif()
if(TARGET Qt::Widgets)
    add_subdirectory(qabstractitemmodeltester)
endif()
