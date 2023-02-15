# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qcoreapplication Test:
#####################################################################

if (WIN32)
    set(target_version "1.2.3.4")
else()
    set(target_version "1.2.3")
endif()

qt_internal_add_test(tst_qcoreapplication
    VERSION ${target_version}
    SOURCES
        tst_qcoreapplication.cpp tst_qcoreapplication.h
    LIBRARIES
        Qt::CorePrivate
)

if (APPLE)
    set_property(TARGET tst_qcoreapplication PROPERTY MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist")
    set_property(TARGET tst_qcoreapplication PROPERTY PROPERTY MACOSX_BUNDLE TRUE)
endif()

if (ANDROID)
    set_property(TARGET tst_qcoreapplication PROPERTY QT_ANDROID_VERSION_NAME ${target_version})
endif()
