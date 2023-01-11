# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_test(tst_manual_qpermissions
    SOURCES
        tst_qpermissions.cpp
    LIBRARIES
        Qt::CorePrivate
)

if(ANDROID)
    set_property(TARGET tst_manual_qpermissions
        PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
elseif(APPLE)
    # Test an app bundle, but without any usage descriptions

    qt_internal_add_test(tst_qpermissions_app
        SOURCES
            tst_qpermissions.cpp
        DEFINES
            tst_QPermissions=tst_QPermissionsApp
        LIBRARIES
            Qt::CorePrivate
    )

    set_property(TARGET tst_qpermissions_app
        PROPERTY MACOSX_BUNDLE TRUE)
    set_property(TARGET tst_qpermissions_app
        PROPERTY MACOSX_BUNDLE_GUI_IDENTIFIER "io.qt.dev.tst_permissions_app")

    # Test an app bundle with all the required usage descriptions

    qt_internal_add_test(tst_qpermissions_app_with_usage_descriptions
        SOURCES
            tst_qpermissions.cpp
        DEFINES
            tst_QPermissions=tst_QPermissionsAppWithUsageDescriptions
            HAVE_USAGE_DESCRIPTION=1
        LIBRARIES
            Qt::CorePrivate
            Qt::Gui
    )

    set_property(TARGET tst_qpermissions_app_with_usage_descriptions
        PROPERTY MACOSX_BUNDLE TRUE)
    set_property(TARGET tst_qpermissions_app_with_usage_descriptions
        PROPERTY MACOSX_BUNDLE_GUI_IDENTIFIER "io.qt.dev.tst_qpermissions_app_with_usage_descriptions")
    set_property(TARGET tst_qpermissions_app_with_usage_descriptions
        PROPERTY MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist")

    foreach(permission_plugin IN LISTS QT_ALL_PLUGINS_FOUND_BY_FIND_PACKAGE_permissions)
        set(permission_plugin "${QT_CMAKE_EXPORT_NAMESPACE}::${permission_plugin}")
        qt6_import_plugins(tst_qpermissions_app INCLUDE ${permission_plugin})
        qt6_import_plugins(tst_qpermissions_app_with_usage_descriptions INCLUDE ${permission_plugin})
    endforeach()

    if(NOT CMAKE_GENERATOR STREQUAL "Xcode")
        add_custom_command(TARGET tst_qpermissions_app_with_usage_descriptions
            POST_BUILD COMMAND codesign -s - tst_qpermissions_app_with_usage_descriptions.app)
    endif()
endif()
