# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qstatictext.pro.

#####################################################################
## tst_qstatictext Test:
#####################################################################

qt_internal_add_test(tst_qstatictext
    SOURCES
        tst_qstatictext.cpp
    LIBRARIES
        Qt::Gui
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qstatictext CONDITION QT_FEATURE_private_tests
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
)
