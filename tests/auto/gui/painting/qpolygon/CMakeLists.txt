# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qpolygon Test:
#####################################################################

qt_internal_add_test(tst_qpolygon
    SOURCES
        tst_qpolygon.cpp
    LIBRARIES
        Qt::Gui
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qpolygon CONDITION UNIX AND NOT APPLE AND NOT HAIKU AND NOT INTEGRITY
    LIBRARIES
        m
)
