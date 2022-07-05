# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qstring.pro.

#####################################################################
## tst_qstring Test:
#####################################################################

qt_internal_add_test(tst_qstring
    SOURCES
        tst_qstring.cpp
    DEFINES
        QT_DISABLE_DEPRECATED_BEFORE=0
    LIBRARIES
        Qt::CorePrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qstring CONDITION NOT QT_FEATURE_doubleconversion AND NOT QT_FEATURE_system_doubleconversion
    DEFINES
        QT_NO_DOUBLECONVERSION
)

qt_internal_extend_target(tst_qstring CONDITION APPLE
    SOURCES
        tst_qstring_mac.mm
    LIBRARIES
        ${FWFoundation}
)
