# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qdatetime.pro.

#####################################################################
## tst_qdatetime Test:
#####################################################################

qt_internal_add_test(tst_qdatetime
    SOURCES
        tst_qdatetime.cpp
    DEFINES
        QT_NO_FOREACH
        QT_NO_KEYWORDS
    LIBRARIES
        Qt::CorePrivate
)

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qdatetime.pro:MSVC:
# QMAKE_CFLAGS_RELEASE = "--O1"
# QMAKE_CXXFLAGS_RELEASE = "--O1"

qt_internal_extend_target(tst_qdatetime CONDITION APPLE
    SOURCES
        tst_qdatetime_mac.mm
    LIBRARIES
        ${FWFoundation}
)
