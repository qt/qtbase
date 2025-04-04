# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qfloat16 Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qfloat16 LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qfloat16
    SOURCES
        tst_qfloat16.cpp
    LIBRARIES
        Qt::TestPrivate
)

# To test std::format support, if possible
if(QT_FEATURE_cxx20_format)
    set_target_properties(tst_qfloat16
        PROPERTIES
            CXX_STANDARD 20
            CXX_STANDARD_REQUIRED OFF
    )
endif()
