# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qlocale Test:
#####################################################################

qt_internal_add_test(tst_qlocale
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../"
    SOURCES
        ../tst_qlocale.cpp
    LIBRARIES
        Qt::CorePrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qlocale CONDITION embedded
    LIBRARIES
        Qt::Gui
)

qt_internal_extend_target(tst_qlocale CONDITION NOT QT_FEATURE_doubleconversion AND NOT QT_FEATURE_system_doubleconversion
    DEFINES
        QT_NO_DOUBLECONVERSION
)

## Depends on ../syslocaleapp
if(QT_FEATURE_process)
    add_dependencies(tst_qlocale syslocaleapp)
endif()
