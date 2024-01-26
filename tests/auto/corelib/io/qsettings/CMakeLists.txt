# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsettings Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qsettings LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Test:
set(qsettings_resource_files
    "bom.ini"
    "resourcefile.ini"
    "resourcefile2.ini"
    "resourcefile3.ini"
    "resourcefile6.plist"
    "withcomments.ini"
    "float.ini"
    "qt5settings.ini"
    "utf8settings.ini"
)

qt_internal_add_test(tst_qsettings
    SOURCES
        tst_qsettings.cpp
    INCLUDE_DIRECTORIES
        ../../kernel/qmetatype
    NO_BATCH # QTBUG-121815
    DEFINES
        QTEST_THROW_ON_FAIL
        QTEST_THROW_ON_SKIP
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
    TESTDATA ${qsettings_resource_files}
    BUILTIN_TESTDATA
)


## Scopes:
#####################################################################

qt_internal_extend_target(tst_qsettings CONDITION MSVC
    LIBRARIES
        advapi32
)

qt_internal_extend_target(tst_qsettings CONDITION APPLE
    LIBRARIES
        ${FWCoreFoundation}
)
