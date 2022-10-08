# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_test(tst_qsharedmemory
    SOURCES
        tst_qsharedmemory.cpp
    LIBRARIES
        Qt::CorePrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qsharedmemory CONDITION LINUX
    LIBRARIES
        rt
)
add_subdirectory(producerconsumer)
if(QT_FEATURE_process)
    add_dependencies(tst_qsharedmemory producerconsumer_helper)
endif()
