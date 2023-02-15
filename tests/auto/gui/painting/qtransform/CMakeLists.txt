# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qtransform Test:
#####################################################################

qt_internal_add_test(tst_qtransform
    SOURCES
        tst_qtransform.cpp
    LIBRARIES
        Qt::Gui
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qtransform CONDITION UNIX AND NOT APPLE AND NOT HAIKU AND NOT INTEGRITY
    LIBRARIES
        m
)
