# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_noqteventloop Test:
#####################################################################

qt_internal_add_test(tst_noqteventloop
    SOURCES
        tst_noqteventloop.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Network
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_noqteventloop CONDITION QT_FEATURE_dynamicgl AND WIN32
    LIBRARIES
        user32
)
