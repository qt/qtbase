macro(qt_find_apple_system_frameworks)
    if(APPLE)
        find_library(FWAppKit AppKit)
        find_library(FWAssetsLibrary AssetsLibrary)
        find_library(FWAudioToolbox AudioToolbox)
        find_library(FWApplicationServices ApplicationServices)
        find_library(FWCarbon Carbon)
        find_library(FWCoreFoundation CoreFoundation)
        find_library(FWCoreServices CoreServices)
        find_library(FWCoreGraphics CoreGraphics)
        find_library(FWCoreText CoreText)
        find_library(FWCoreVideo CoreVideo)
        find_library(FWcups cups)
        find_library(FWDiskArbitration DiskArbitration)
        find_library(FWFoundation Foundation)
        find_library(FWIOBluetooth IOBluetooth)
        find_library(FWIOKit IOKit)
        find_library(FWIOSurface IOSurface)
        find_library(FWImageIO ImageIO)
        find_library(FWMetal Metal)
        find_library(FWMobileCoreServices MobileCoreServices)
        find_library(FWQuartzCore QuartzCore)
        find_library(FWSecurity Security)
        find_library(FWSystemConfiguration SystemConfiguration)
        find_library(FWUIKit UIKit)
        find_library(FWWatchKit WatchKit)
        find_library(FWGameController GameController)
    endif()
endmacro()

# Copy header files to QtXYZ.framework/Versions/6/Headers/
# Use this function for header files that
#   - are not added as source files to the target
#   - are not marked as PUBLIC_HEADER
#   - or are private and supposed to end up in the 6.7.8/QtXYZ/private subdir.
function(qt_copy_framework_headers target)
    get_target_property(is_fw ${target} FRAMEWORK)
    if(NOT "${is_fw}")
        return()
    endif()

    set(options PUBLIC PRIVATE QPA)
    set(oneValueArgs)
    set(multiValueArgs)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    get_target_property(fw_version ${target} FRAMEWORK_VERSION)
    get_target_property(fw_bundle_version ${target} MACOSX_FRAMEWORK_BUNDLE_VERSION)
    get_target_property(fw_dir ${target} LIBRARY_OUTPUT_DIRECTORY)
    get_target_property(fw_name ${target} OUTPUT_NAME)
    set(fw_headers_dir ${fw_dir}/${fw_name}.framework/Versions/${fw_version}/Headers/)
    if(ARG_PRIVATE)
        string(APPEND fw_headers_dir "${fw_bundle_version}/Qt${target}/private/")
    elseif(ARG_QPA)
        string(APPEND fw_headers_dir "${fw_bundle_version}/Qt${target}/qpa/")
    endif()

    set(out_files)
    foreach(hdr IN LISTS ARG_UNPARSED_ARGUMENTS)
        get_filename_component(in_file_path ${hdr} ABSOLUTE)
        get_filename_component(in_file_name ${hdr} NAME)
        set(out_file_path ${fw_headers_dir}${in_file_name})
        add_custom_command(
            OUTPUT ${out_file_path}
            DEPENDS ${in_file_path}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${fw_headers_dir}"
            COMMAND ${CMAKE_COMMAND} -E copy "${in_file_path}" "${fw_headers_dir}")
        list(APPEND out_files ${out_file_path})
    endforeach()

    get_target_property(fw_copied_headers ${target} QT_COPIED_FRAMEWORK_HEADERS)
    if(NOT fw_copied_headers)
        set(fw_copied_headers "")
    endif()
    list(APPEND fw_copied_headers ${out_files})
    set_target_properties(${target} PROPERTIES QT_COPIED_FRAMEWORK_HEADERS "${fw_copied_headers}")
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
        # Hack to create the "Headers" symlink in the framework:
        # Create a fake header file and copy it into the framework by marking it as PUBLIC_HEADER.
        # CMake now takes care of creating the symlink.
        set(fake_header ${target}_fake_header.h)
        qt_get_main_cmake_configuration(main_config)
        file(GENERATE OUTPUT ${fake_header} CONTENT "// ignore this file\n"
             CONDITION "$<CONFIG:${main_config}>")
        string(PREPEND fake_header "${CMAKE_CURRENT_BINARY_DIR}/")
        target_sources(${target} PRIVATE ${fake_header})
        set_source_files_properties(${fake_header} PROPERTIES GENERATED ON)
        set_property(TARGET ${target} APPEND PROPERTY PUBLIC_HEADER ${fake_header})

        # Add a target, e.g. Core_framework_headers, that triggers the header copy.
        add_custom_target(${target}_framework_headers DEPENDS ${headers})
        add_dependencies(${target} ${target}_framework_headers)
    endif()
endfunction()
