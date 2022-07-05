# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#####################################################################
## tst_qsystemsemaphore Test:
#####################################################################

qt_internal_add_test(tst_qsystemsemaphore
    SOURCES
        tst_qsystemsemaphore.cpp
)

add_subdirectory(acquirerelease)
if(QT_FEATURE_process)
    add_dependencies(tst_qsystemsemaphore acquirerelease_helper)
endif()
