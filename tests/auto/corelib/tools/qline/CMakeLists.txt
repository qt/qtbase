# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qline.pro.

#####################################################################
## tst_qline Test:
#####################################################################

qt_internal_add_test(tst_qline
    SOURCES
        tst_qline.cpp
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qline CONDITION UNIX AND NOT APPLE AND NOT HAIKU AND NOT INTEGRITY AND NOT VXWORKS
    LIBRARIES
        m
)
