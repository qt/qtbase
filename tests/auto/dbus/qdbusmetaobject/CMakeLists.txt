# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdbusmetaobject Test:
#####################################################################

qt_internal_add_test(tst_qdbusmetaobject
    SOURCES
        tst_qdbusmetaobject.cpp
    LIBRARIES
        Qt::DBusPrivate
)
