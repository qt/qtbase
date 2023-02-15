# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdbusxmlparser Test:
#####################################################################

qt_internal_add_test(tst_qdbusxmlparser
    SOURCES
        tst_qdbusxmlparser.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::DBusPrivate
        Qt::Xml
)
