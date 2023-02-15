# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_network_remote_stresstest Binary:
#####################################################################

qt_internal_add_manual_test(tst_network_remote_stresstest
    SOURCES
        tst_network_remote_stresstest.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Network
        Qt::NetworkPrivate
        Qt::Test
)

# Resources:
set(url-list_resource_files
    "url-list.txt"
)

qt_internal_add_resource(tst_network_remote_stresstest "url-list"
    PREFIX
        "/"
    FILES
        ${url-list_resource_files}
)

