# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qabstractnetworkcache Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    tests/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qabstractnetworkcache
    SOURCES
        tst_qabstractnetworkcache.cpp
    LIBRARIES
        Qt::Network
    TESTDATA ${test_data}
    QT_TEST_SERVER_LIST "apache2"
)

# QT_TEST_SERVER_LIST = "apache2"
