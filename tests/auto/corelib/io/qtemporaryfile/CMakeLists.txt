# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qtemporaryfile Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qtemporaryfile LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
list(APPEND test_data "tst_qtemporaryfile.cpp")

qt_internal_add_test(tst_qtemporaryfile
    SOURCES
        tst_qtemporaryfile.cpp
    LIBRARIES
        Qt::TestPrivate
    TESTDATA ${test_data}
)

# Resources:
set(qtemporaryfile_resource_files
    "resources/test.txt"
)

qt_internal_add_resource(tst_qtemporaryfile "qtemporaryfile"
    PREFIX
        "/"
    FILES
        ${qtemporaryfile_resource_files}
)


## Scopes:
#####################################################################

if(ANDROID)
    # Resources:
    set(android_testdata_resource_files
        "resources/test.txt"
        "tst_qtemporaryfile.cpp"
    )

    qt_internal_add_resource(tst_qtemporaryfile "android_testdata"
        PREFIX
            "/android_testdata"
        FILES
            ${android_testdata_resource_files}
    )
endif()

qt_internal_extend_target(tst_qtemporaryfile CONDITION WIN32
    LIBRARIES
        shlwapi
)
