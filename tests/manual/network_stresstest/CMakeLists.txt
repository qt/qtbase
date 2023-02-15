# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_network_stresstest Binary:
#####################################################################

qt_internal_add_manual_test(tst_network_stresstest
    SOURCES
        minihttpserver.cpp minihttpserver.h
        tst_network_stresstest.cpp
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
