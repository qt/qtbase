# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# For now these are simple internal forwarding wrappers for the public counterparts, which are
# meant to be used in qt repo CMakeLists.txt files.
function(qt_internal_add_sbom)
    _qt_internal_add_sbom(${ARGN})
endfunction()

function(qt_internal_extend_sbom)
    _qt_internal_extend_sbom(${ARGN})
endfunction()

function(qt_internal_sbom_add_license)
    _qt_internal_sbom_add_license(${ARGN})
endfunction()

function(qt_internal_extend_sbom_dependencies)
    _qt_internal_extend_sbom_dependencies(${ARGN})
endfunction()

function(qt_find_package_extend_sbom)
    _qt_find_package_extend_sbom(${ARGN})
endfunction()

function(qt_internal_sbom_begin_qt_repo_project)
    _qt_internal_sbom_begin_qt_repo_project(${ARGN})
endfunction()

function(qt_internal_sbom_end_qt_repo_project)
    _qt_internal_sbom_end_qt_repo_project(${ARGN})
endfunction()

function(qt_internal_sbom_add_files)
    _qt_internal_sbom_add_files(${ARGN})
endfunction()

function(qt_internal_sbom_add_cmake_include_step)
    _qt_internal_sbom_add_cmake_include_step(${ARGN})
endfunction()

function(qt_internal_sbom_add_external_reference)
    _qt_internal_sbom_generate_add_external_reference(${ARGN})
endfunction()

function(qt_internal_sbom_add_project_relationship)
    _qt_internal_sbom_generate_add_project_relationship(${ARGN})
endfunction()

function(qt_internal_sbom_generate_tag_value_spdx_document)
    _qt_internal_sbom_generate_tag_value_spdx_document(${ARGN})

    set(opt_args "")
    set(single_args
        OUT_VAR_OUTPUT_FILE_NAME
        OUT_VAR_OUTPUT_ABSOLUTE_FILE_PATH
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")

    if(arg_OUT_VAR_OUTPUT_FILE_NAME)
        set(${arg_OUT_VAR_OUTPUT_FILE_NAME} "${${arg_OUT_VAR_OUTPUT_FILE_NAME}}" PARENT_SCOPE)
    endif()

    if(arg_OUT_VAR_OUTPUT_ABSOLUTE_FILE_PATH)
        set(${arg_OUT_VAR_OUTPUT_ABSOLUTE_FILE_PATH} "${${arg_OUT_VAR_OUTPUT_ABSOLUTE_FILE_PATH}}"
            PARENT_SCOPE)
    endif()
endfunction()
