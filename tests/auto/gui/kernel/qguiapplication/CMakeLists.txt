# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qguiapplication Test:
#####################################################################

if (WIN32)
    set(target_version "1.2.3.4")
else()
    set(target_version "1.2.3")
endif()

# Resources:
set(tst_qguiapplication_resource_files
    "icons/appicon.png"
    "icons/usericon.png"
)

qt_internal_add_test(tst_qguiapplication
    VERSION ${target_version}
    SOURCES
        ../../../corelib/kernel/qcoreapplication/tst_qcoreapplication.cpp ../../../corelib/kernel/qcoreapplication/tst_qcoreapplication.h
        tst_qguiapplication.cpp
    DEFINES
        QT_QGUIAPPLICATIONTEST=1
    INCLUDE_DIRECTORIES
        ../../../corelib/kernel/qcoreapplication
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    TESTDATA ${tst_qguiapplication_resource_files}
    BUILTIN_TESTDATA
)

if (ANDROID)
    set_property(TARGET tst_qguiapplication PROPERTY QT_ANDROID_VERSION_NAME ${target_version})
endif()
if (APPLE)
    set_property(TARGET tst_qguiapplication PROPERTY MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist")
    set_property(TARGET tst_qguiapplication PROPERTY PROPERTY MACOSX_BUNDLE TRUE)
endif()
