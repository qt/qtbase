# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(test_qt_extract_metatypes VERSION 0.1 LANGUAGES CXX)

set(test_project_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/test_qt_extract_metatypes_project")
set(test_project_build_dir "${CMAKE_CURRENT_BINARY_DIR}/build_qt_extract_metatypes_test_project")

# Make sure that file paths are 'real' paths
get_filename_component(test_project_source_dir "${test_project_source_dir}" REALPATH)
get_filename_component(test_project_build_dir "${test_project_build_dir}" REALPATH)

get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)
if (CMAKE_BUILD_TYPE AND NOT is_multi_config)
    string(TOLOWER "qt6metatypetest_${CMAKE_BUILD_TYPE}" metatypes_file_basename)
else()
    string(TOLOWER "qt6metatypetest" metatypes_file_basename)
endif()
set(meta_types_file
    "${test_project_build_dir}/meta_types/${metatypes_file_basename}_metatypes.json")

file(REMOVE_RECURSE "${test_project_build_dir}")
file(MAKE_DIRECTORY "${test_project_build_dir}")

find_package(Qt6 COMPONENTS Core REQUIRED)

include("${_Qt6CTestMacros}")

macro(try_build)
    execute_process(COMMAND
            "${CMAKE_COMMAND}"
            --build "${test_project_build_dir}"
        RESULT_VARIABLE result
    )
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Unable to build test project")
    endif()
endmacro()

macro(copy_test_header header)
    file(COPY "${test_project_source_dir}/testdata/${header}"
        DESTINATION "${test_project_build_dir}")
    file(RENAME "${test_project_build_dir}/${header}" "${test_project_build_dir}/MetaType.h")
    file(TOUCH "${test_project_build_dir}/MetaType.h")
endmacro()

macro(check_generated_metatypes_file reference expect)
    set(reference_meta_types_file "${test_project_source_dir}/testdata/${reference}")
    execute_process(COMMAND
            "${CMAKE_COMMAND}"
            -E compare_files "${meta_types_file}" "${reference_meta_types_file}"
        RESULT_VARIABLE compare_result
    )
    if(NOT compare_result EQUAL ${expect})
        message(FATAL_ERROR "${meta_types_file} and ${reference_meta_types_file} content differs")
    endif()
    unset(reference_meta_types_file)
endmacro()

copy_test_header(MetaTypeQ_OBJECT.h)

_qt_internal_get_cmake_test_configure_options(option_list)
execute_process(COMMAND
        "${CMAKE_COMMAND}"
        "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}"
        "-G${CMAKE_GENERATOR}"
        ${option_list}
        -B "${test_project_build_dir}"
        -S "${test_project_source_dir}"
    RESULT_VARIABLE result
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Unable to configure test project")
endif()

try_build()
check_generated_metatypes_file(qt6metatypetest_metatypesQ_OBJECT.json 0)

copy_test_header(MetaTypeQ_OBJECTandQ_PROPERTY.h)
try_build()
check_generated_metatypes_file(qt6metatypetest_metatypesQ_OBJECTandQ_PROPERTY.json 0)

copy_test_header(MetaTypeEmpty.h)
try_build()
check_generated_metatypes_file(qt6metatypetest_metatypesEmpty.json 0)
file(TIMESTAMP "${meta_types_file}" metatypes_timestamp)

copy_test_header(MetaTypeEmptyWithComment.h)
try_build()
check_generated_metatypes_file(qt6metatypetest_metatypesEmpty.json 0)

file(TIMESTAMP "${meta_types_file}" new_metatypes_timestamp)

# Depending on the way how qt_extract_metatypes executes automoc it might or might not
# change the resulting .json file content.
set(extract_metatypes_uses_dep_files FALSE)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.17") # Requires automoc changes present only in 3.17
    if(CMAKE_GENERATOR STREQUAL "Ninja" OR CMAKE_GENERATOR STREQUAL "Ninja Multi-Config")
        set(extract_metatypes_uses_dep_files TRUE)
    endif()
endif()

if(extract_metatypes_uses_dep_files)
    if(NOT metatypes_timestamp STREQUAL new_metatypes_timestamp)
        message(FATAL_ERROR "${meta_types_file} timestamp is changed but should not")
    endif()
endif()
