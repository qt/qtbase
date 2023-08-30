# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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

qt_internal_extend_target(tst_qstatictext CONDITION QT_FEATURE_developer_build
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
)
