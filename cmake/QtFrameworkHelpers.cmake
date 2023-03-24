# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

macro(qt_find_apple_system_frameworks)
    if(APPLE)
        qt_internal_find_apple_system_framework(FWAppKit AppKit)
        qt_internal_find_apple_system_framework(FWAssetsLibrary AssetsLibrary)
        qt_internal_find_apple_system_framework(FWPhotos Photos)
        qt_internal_find_apple_system_framework(FWAudioToolbox AudioToolbox)
        qt_internal_find_apple_system_framework(FWApplicationServices ApplicationServices)
        qt_internal_find_apple_system_framework(FWCarbon Carbon)
        qt_internal_find_apple_system_framework(FWCoreFoundation CoreFoundation)
        qt_internal_find_apple_system_framework(FWCoreServices CoreServices)
        qt_internal_find_apple_system_framework(FWCoreGraphics CoreGraphics)
        qt_internal_find_apple_system_framework(FWCoreText CoreText)
        qt_internal_find_apple_system_framework(FWCoreVideo CoreVideo)
        qt_internal_find_apple_system_framework(FWCryptoTokenKit CryptoTokenKit)
        qt_internal_find_apple_system_framework(FWDiskArbitration DiskArbitration)
        qt_internal_find_apple_system_framework(FWFoundation Foundation)
        qt_internal_find_apple_system_framework(FWIOBluetooth IOBluetooth)
        qt_internal_find_apple_system_framework(FWIOKit IOKit)
        qt_internal_find_apple_system_framework(FWIOSurface IOSurface)
        qt_internal_find_apple_system_framework(FWImageIO ImageIO)
        qt_internal_find_apple_system_framework(FWMetal Metal)
        qt_internal_find_apple_system_framework(FWMobileCoreServices MobileCoreServices)
        qt_internal_find_apple_system_framework(FWQuartzCore QuartzCore)
        qt_internal_find_apple_system_framework(FWSecurity Security)
        qt_internal_find_apple_system_framework(FWSystemConfiguration SystemConfiguration)
        qt_internal_find_apple_system_framework(FWUIKit UIKit)
        qt_internal_find_apple_system_framework(FWCoreLocation CoreLocation)
        qt_internal_find_apple_system_framework(FWCoreMotion CoreMotion)
        qt_internal_find_apple_system_framework(FWWatchKit WatchKit)
        qt_internal_find_apple_system_framework(FWGameController GameController)
        qt_internal_find_apple_system_framework(FWCoreBluetooth CoreBluetooth)
        qt_internal_find_apple_system_framework(FWAVFoundation AVFoundation)
        qt_internal_find_apple_system_framework(FWContacts Contacts)
        qt_internal_find_apple_system_framework(FWEventKit EventKit)
        qt_internal_find_apple_system_framework(FWHealthKit HealthKit)
    endif()
endmacro()

# Given framework_name == 'IOKit', sets non-cache variable 'FWIOKit' to '-framework IOKit' in
# the calling directory scope if the framework is found, or 'IOKit-NOTFOUND'.
function(qt_internal_find_apple_system_framework out_var framework_name)
    # To avoid creating many FindFoo.cmake files for each apple system framework, populate each
    # FWFoo variable with '-framework Foo' instead of an absolute path to the framework. This makes
    # the generated CMake target files relocatable, so that Xcode SDK absolute paths are not
    # hardcoded, like with Xcode11.app on the CI.
    # We might revisit this later.
    set(cache_var_name "${out_var}Internal")

    find_library(${cache_var_name} "${framework_name}")

    if(${cache_var_name} AND ${cache_var_name} MATCHES ".framework$")
        set(${out_var} "-framework ${framework_name}" PARENT_SCOPE)
    else()
        set(${out_var} "${out_var}-NOTFOUND" PARENT_SCOPE)
    endif()
endfunction()

# Copy header files to QtXYZ.framework/Versions/A/Headers/
# Use this function for header files that
#   - are not added as source files to the target
#   - are not marked as PUBLIC_HEADER
#   - or are private and supposed to end up in the 6.7.8/QtXYZ/private subdir.
function(qt_copy_framework_headers target)
    get_target_property(is_fw ${target} FRAMEWORK)
    if(NOT "${is_fw}")
        return()
    endif()

    set(options)
    set(oneValueArgs)
    set(multiValueArgs PUBLIC PRIVATE QPA)
    cmake_parse_arguments(arg "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    qt_internal_get_framework_info(fw ${target})
    get_target_property(output_dir ${target} LIBRARY_OUTPUT_DIRECTORY)
    set(output_dir_PUBLIC "${output_dir}/${fw_versioned_header_dir}")
    set(output_dir_PRIVATE "${output_dir}/${fw_private_module_header_dir}/private")
    set(output_dir_QPA "${output_dir}/${fw_private_module_header_dir}/qpa")


    set(out_files)
    foreach(type IN ITEMS PUBLIC PRIVATE QPA)
        set(fw_output_header_dir "${output_dir_${type}}")
        foreach(hdr IN LISTS arg_${type})
            get_filename_component(in_file_path ${hdr} ABSOLUTE)
            get_filename_component(in_file_name ${hdr} NAME)
            set(out_file_path "${fw_output_header_dir}/${in_file_name}")
            add_custom_command(
                OUTPUT ${out_file_path}
                DEPENDS ${in_file_path}
                COMMAND ${CMAKE_COMMAND} -E make_directory "${fw_output_header_dir}"
                COMMAND ${CMAKE_COMMAND} -E copy "${in_file_path}" "${fw_output_header_dir}"
                VERBATIM)
            list(APPEND out_files ${out_file_path})
        endforeach()
    endforeach()

    set_property(TARGET ${target} APPEND PROPERTY
        QT_COPIED_FRAMEWORK_HEADERS "${out_files}")
endfunction()

function(qt_internal_generate_fake_framework_header target)
    # Hack to create the "Headers" symlink in the framework:
    # Create a fake header file and copy it into the framework by marking it as PUBLIC_HEADER.
    # CMake now takes care of creating the symlink.
    set(fake_header "${CMAKE_CURRENT_BINARY_DIR}/${target}_fake_header.h")
    qt_internal_get_main_cmake_configuration(main_config)
    file(GENERATE OUTPUT "${fake_header}" CONTENT "// ignore this file\n"
        CONDITION "$<CONFIG:${main_config}>")
    target_sources(${target} PRIVATE "${fake_header}")
    set_source_files_properties("${fake_header}" PROPERTIES GENERATED ON)
    set_property(TARGET ${target} APPEND PROPERTY PUBLIC_HEADER "${fake_header}")
endfunction()

function(qt_finalize_framework_headers_copy target)
    get_target_property(target_type ${target} TYPE)
    if(${target_type} STREQUAL "INTERFACE_LIBRARY")
        return()
    endif()
    get_target_property(is_fw ${target} FRAMEWORK)
    if(NOT "${is_fw}")
        return()
    endif()
    get_target_property(headers ${target} QT_COPIED_FRAMEWORK_HEADERS)
    if(headers)
        qt_internal_generate_fake_framework_header(${target})

        # Add a target, e.g. Core_framework_headers, that triggers the header copy.
        add_custom_target(${target}_framework_headers DEPENDS ${headers})
        add_dependencies(${target} ${target}_framework_headers)
    endif()
endfunction()

# Collects the framework related information and paths from the target properties.
# Output variables:
#    <out_var>_name framework base name, e.g. 'QtCore'.
#    <out_var>_dir framework base directory, e.g. 'QtCore.framework'.
#    <out_var>_version framework version, e.g. 'A', 'B' etc.
#    <out_var>_bundle_version framework bundle version, same as the PROJECT_VERSION, e.g. '6.0.0'.
#    <out_var>_header_dir top-level header directory, e.g. 'QtCore.framework/Headers'.
#    <out_var>_versioned_header_dir header directory for specific framework version,
#        e.g. 'QtCore.framework/Versions/A/Headers'
#    <out_var>_private_header_dir header directory for the specific framework version and
#       framework bundle version e.g. 'QtCore.framework/Versions/A/Headers/6.0.0'
#    <out_var>_private_module_header_dir private header directory for the specific framework
#       version, framework bundle version and tailing module name, e.g.
#       'QtCore.framework/Versions/A/Headers/6.0.0/Core'
function(qt_internal_get_framework_info out_var target)
    get_target_property(${out_var}_version ${target} FRAMEWORK_VERSION)
    get_target_property(${out_var}_bundle_version ${target} MACOSX_FRAMEWORK_BUNDLE_VERSION)

    # The module name might be different of the actual target name
    # and we want to use the Qt'fied module name as a framework identifier.
    get_target_property(module_interface_name ${target} _qt_module_interface_name)
    if(module_interface_name)
        qt_internal_qtfy_target(module ${module_interface_name})
    else()
        qt_internal_qtfy_target(module ${target})
    endif()

    set(${out_var}_name "${module}")
    set(${out_var}_dir "${${out_var}_name}.framework")
    set(${out_var}_header_dir "${${out_var}_dir}/Headers")
    set(${out_var}_versioned_header_dir "${${out_var}_dir}/Versions/${${out_var}_version}/Headers")
    set(${out_var}_private_header_dir "${${out_var}_header_dir}/${${out_var}_bundle_version}")
    set(${out_var}_private_module_header_dir "${${out_var}_private_header_dir}/${module}")

    set(${out_var}_name "${${out_var}_name}" PARENT_SCOPE)
    set(${out_var}_dir "${${out_var}_dir}" PARENT_SCOPE)
    set(${out_var}_header_dir "${${out_var}_header_dir}" PARENT_SCOPE)
    set(${out_var}_version "${${out_var}_version}" PARENT_SCOPE)
    set(${out_var}_bundle_version "${${out_var}_bundle_version}" PARENT_SCOPE)
    set(${out_var}_versioned_header_dir "${${out_var}_versioned_header_dir}" PARENT_SCOPE)
    set(${out_var}_private_header_dir "${${out_var}_private_header_dir}" PARENT_SCOPE)
    set(${out_var}_private_module_header_dir "${${out_var}_private_module_header_dir}" PARENT_SCOPE)
endfunction()
