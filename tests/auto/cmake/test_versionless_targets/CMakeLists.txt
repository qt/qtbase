# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(versionless_targets)

function(check_versionless_targets)
    set(known_interface_properties
        QT_MAJOR_VERSION
        AUTOMOC_MACRO_NAMES
        AUTOUIC_OPTIONS
        COMPILE_DEFINITIONS
        COMPILE_FEATURES
        COMPILE_OPTIONS
        CXX_MODULE_SETS
        HEADER_SETS
        HEADER_SETS_TO_VERIFY
        INCLUDE_DIRECTORIES
        LINK_DEPENDS
        LINK_DIRECTORIES
        LINK_LIBRARIES
        LINK_LIBRARIES_DIRECT
        LINK_LIBRARIES_DIRECT_EXCLUDE
        LINK_OPTIONS
        POSITION_INDEPENDENT_CODE
        PRECOMPILE_HEADERS
        SOURCES
        SYSTEM_INCLUDE_DIRECTORIES
    )

    set(known_qt_exported_properties
        MODULE_PLUGIN_TYPES
        QT_DISABLED_PRIVATE_FEATURES
        QT_DISABLED_PUBLIC_FEATURES
        QT_ENABLED_PRIVATE_FEATURES
        QT_ENABLED_PUBLIC_FEATURES
        QT_QMAKE_PRIVATE_CONFIG
        QT_QMAKE_PUBLIC_CONFIG
        QT_QMAKE_PUBLIC_QT_CONFIG
        _qt_config_module_name
        _qt_is_public_module
        _qt_module_has_headers
        _qt_module_has_private_headers
        _qt_module_has_public_headers
        _qt_module_has_qpa_headers
        _qt_module_has_rhi_headers
        _qt_module_include_name
        _qt_module_interface_name
        _qt_package_name
        _qt_package_version
        _qt_private_module_target_name
    )

    foreach(prop ${known_interface_properties})
        set(versionless_prop "")
        set(versioned_prop "")
        get_target_property(versionless_prop Qt::Core INTERFACE_${prop})
        get_target_property(versioned_prop Qt6::Core INTERFACE_${prop})
        if(NOT versionless_prop AND NOT versioned_prop)
            continue()
        endif()

        if(NOT "${versionless_prop}" STREQUAL "${versioned_prop}")
            message(SEND_ERROR "INTERFACE_${prop} doesn't match versionless ${versionless_prop}"
                " versioned ${versioned_prop}")
        endif()
    endforeach()

    foreach(prop ${known_qt_exported_properties})
        set(versionless_prop "")
        set(versioned_prop "")
        get_target_property(versionless_prop Qt::Core ${prop})
        get_target_property(versioned_prop Qt6::Core ${prop})
        if(NOT versionless_prop AND NOT versioned_prop)
            continue()
        endif()

        if(NOT "${versionless_prop}" STREQUAL "${versioned_prop}")
            message(SEND_ERROR "${prop} doesn't match versionless ${versionless_prop}"
                " versioned ${versioned_prop}")
        endif()
    endforeach()

    foreach(conf "" _RELEASE _DEBUG _RELWITHDEBINFO _MINSIZEREL)
        set(versionless_prop "")
        set(versioned_prop "")
        get_target_property(versionless_prop Qt::Core IMPORTED_LOCATION${conf})
        get_target_property(versioned_prop Qt6::Core IMPORTED_LOCATION${conf})
        if(NOT versionless_prop AND NOT versioned_prop)
            continue()
        endif()
        if(NOT "${versionless_prop}" STREQUAL "${versioned_prop}")
            message(SEND_ERROR "IMPORTED_LOCATION${conf} doesn't match versionless ${versionless_prop}"
                " versioned ${versioned_prop}")
        endif()
    endforeach()
endfunction()

add_subdirectory(default)
add_subdirectory(force_off)
add_subdirectory(force_on)
add_subdirectory(force_old)
