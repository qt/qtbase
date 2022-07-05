# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#####################################################################
## tst_qthreadstorage Test:
#####################################################################

qt_internal_add_test(tst_qthreadstorage
    SOURCES
        tst_qthreadstorage.cpp
)

## Scopes:
#####################################################################

if(NOT ANDROID)
    add_subdirectory(crashonexit)
    if(QT_FEATURE_process)
        add_dependencies(tst_qthreadstorage crashOnExit_helper)
    endif()
endif()
