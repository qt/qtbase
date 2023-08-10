# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdbuscpp2xml Test:
#####################################################################

qt_internal_add_test(tst_qdbuscpp2xml
    SOURCES
        test1.h
        tst_qdbuscpp2xml.cpp
    LIBRARIES
        Qt::DBusPrivate
)

# Resources:
set(qdbuscpp2xml_resource_files
    "test1.h"
)

qt_internal_add_resource(tst_qdbuscpp2xml "qdbuscpp2xml"
    PREFIX
        "/tst_qdbuscpp2xml/"
    FILES
        ${qdbuscpp2xml_resource_files}
)

