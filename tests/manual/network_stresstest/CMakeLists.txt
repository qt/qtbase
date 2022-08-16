# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from network_stresstest.pro.

#####################################################################
## tst_network_stresstest Binary:
#####################################################################

qt_internal_add_manual_test(tst_network_stresstest
    SOURCES
        minihttpserver.cpp minihttpserver.h
        tst_network_stresstest.cpp
    DEFINES
        QT_DISABLE_DEPRECATED_UP_TO=0
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
        Qt::Test
)

# Resources:
set(wwwfiles_resource_files
    "qtest"
)

qt_internal_add_resource(tst_network_stresstest "wwwfiles"
    PREFIX
        "/"
    FILES
        ${wwwfiles_resource_files}
)


#### Keys ignored in scope 1:.:.:network_stresstest.pro:<TRUE>:
# QMAKE_RESOURCE_FLAGS = "-no-compress"
