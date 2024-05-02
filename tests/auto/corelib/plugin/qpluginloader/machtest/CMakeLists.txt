# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause
set_directory_properties(PROPERTIES
    _qt_good_targets ""
    _qt_stub_targets ""
)

function(add_plugin_binary)
    set(no_value_options "")
    set(single_value_options NAME ARCH OUT_TARGET)
    set(multi_value_options SOURCES)
    cmake_parse_arguments(PARSE_ARGV 0 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )

    set(output_name ${arg_NAME}.${arg_ARCH})
    set(target tst_qpluginloader.${output_name})
    set(${arg_OUT_TARGET} ${target} PARENT_SCOPE)
    set_property(DIRECTORY APPEND PROPERTY _qt_${arg_NAME}_targets ${target})
    add_library(${target} MODULE ${arg_SOURCES})
    add_dependencies(tst_qpluginloader ${target})
    set_target_properties(${target} PROPERTIES
        OUTPUT_NAME ${output_name}
        PREFIX ""
        SUFFIX ".dylib"
        DEBUG_POSTFIX ""
        OSX_ARCHITECTURES ${arg_ARCH}
    )
endfunction()

function(add_good_binary)
    set(no_value_options "")
    set(single_value_options ARCH)
    set(multi_value_options "")
    cmake_parse_arguments(PARSE_ARGV 0 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )

    add_plugin_binary(
        NAME good
        ARCH ${arg_ARCH}
        SOURCES ../fakeplugin.cpp
        OUT_TARGET target
    )

    # We cannot link against Qt6::Core, because the architecture might not match.
    # Extract the include directories from Qt6::Core.
    get_target_property(incdirs Qt6::Core INTERFACE_INCLUDE_DIRECTORIES)
    target_include_directories(${target} PRIVATE ${incdirs})

    # Extract the compile definitions from Qt6::Core and disable version tagging.
    get_target_property(compdefs Qt6::Core INTERFACE_COMPILE_DEFINITIONS)
    target_compile_definitions(${target} PRIVATE
        ${compdefs}
        QT_NO_VERSION_TAGGING
    )
endfunction()

function(add_stub_binary)
    set(no_value_options "")
    set(single_value_options ARCH)
    set(multi_value_options "")
    cmake_parse_arguments(PARSE_ARGV 0 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )

    add_plugin_binary(
        NAME stub
        ARCH ${arg_ARCH}
        SOURCES stub.cpp
    )
endfunction()

function(add_fat_binary)
    set(no_value_options "")
    set(single_value_options NAME OUT_TARGET)
    set(multi_value_options TARGETS)
    cmake_parse_arguments(PARSE_ARGV 0 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )

    set(arch_args "")
    foreach(dependency IN LISTS arg_TARGETS)
        get_target_property(arch ${dependency} OSX_ARCHITECTURES)
        list(APPEND arch_args -arch ${arch} $<TARGET_FILE_NAME:${dependency}>)
    endforeach()

    set(output_name good.fat.${arg_NAME})
    set(output_file ${output_name}.dylib)
    set(target tst_qpluginloader.${output_name})
    set(${arg_OUT_TARGET} ${target} PARENT_SCOPE)
    add_custom_command(
        OUTPUT ${output_file}
        COMMAND lipo -create -output ${output_file} ${arch_args}
        DEPENDS ${arg_TARGETS}
    )
    add_custom_target(${target}
        DEPENDS ${output_file}
    )
    add_dependencies(tst_qpluginloader ${target})
endfunction()

set(archs_to_test arm64 x86_64)
foreach(arch IN LISTS archs_to_test)
    add_good_binary(ARCH ${arch})
    add_stub_binary(ARCH ${arch})
endforeach()

get_directory_property(good_targets _qt_good_targets)
add_fat_binary(NAME all TARGETS ${good_targets})

set(targets ${good_targets})
list(FILTER targets EXCLUDE REGEX "\\.arm64$")
add_fat_binary(NAME no-arm64 TARGETS ${targets})

set(targets ${good_targets})
list(FILTER targets EXCLUDE REGEX "\\.x86_64$")
add_fat_binary(NAME no-x86_64 TARGETS ${targets})

get_directory_property(stub_targets _qt_stub_targets)
set(targets ${stub_targets})
list(FILTER targets INCLUDE REGEX "\\.arm64$")
add_fat_binary(NAME stub-arm64 TARGETS ${targets})

set(targets ${stub_targets})
list(FILTER targets INCLUDE REGEX "\\.x86_64$")
add_fat_binary(NAME stub-x86_64 TARGETS ${targets})

set(bad_binary_names "")
foreach(i RANGE 1 13)
    list(APPEND bad_binary_names "bad${i}.dylib")
endforeach()
add_custom_command(
    OUTPUT ${bad_binary_names}
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/generate-bad.pl
)
add_custom_target(tst_qpluginloader_generate_bad_binaries
    DEPENDS ${bad_binary_names}
)
add_dependencies(tst_qpluginloader tst_qpluginloader_generate_bad_binaries)
