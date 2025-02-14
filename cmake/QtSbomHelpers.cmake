# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# These internal sbom functions are meant to be used in qt repo CMakeLists.txt files.

function(qt_internal_add_sbom target)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    qt_internal_sbom_get_default_sbom_args("${target}" sbom_extra_args ${ARGN})
    _qt_internal_add_sbom(${target} ${ARGN} ${sbom_extra_args})
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
        OUT_VAR_DEPS_FOUND
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

    if(arg_OUT_VAR_DEPS_FOUND)
        set(${arg_OUT_VAR_DEPS_FOUND} "${${arg_OUT_VAR_DEPS_FOUND}}" PARENT_SCOPE)
    endif()
endfunction()

function(qt_internal_sbom_verify_deps_for_generate_tag_value_spdx_document)
    _qt_internal_sbom_verify_deps_for_generate_tag_value_spdx_document(${ARGN})

    set(opt_args "")
    set(single_args
        OUT_VAR_DEPS_FOUND
        OUT_VAR_REASON_FAILURE_MESSAGE
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")

    if(arg_OUT_VAR_DEPS_FOUND)
        set(${arg_OUT_VAR_DEPS_FOUND} "${${arg_OUT_VAR_DEPS_FOUND}}" PARENT_SCOPE)
    endif()

    if(arg_OUT_VAR_REASON_FAILURE_MESSAGE)
        set(${arg_OUT_VAR_REASON_FAILURE_MESSAGE}
            "${${arg_OUT_VAR_REASON_FAILURE_MESSAGE}}" PARENT_SCOPE)
    endif()
endfunction()

function(qt_internal_sbom_get_project_spdx_id out_var)
    set(result "")

    if(QT_GENERATE_SBOM)
        get_cmake_property(project_spdx_id _qt_internal_sbom_project_spdx_id)
        if(project_spdx_id)
            set(result "${project_spdx_id}")
        endif()
    endif()

    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

function(qt_internal_sbom_get_target_spdx_id target out_var)
    set(result "")

    if(QT_GENERATE_SBOM)
        _qt_internal_sbom_get_spdx_id_for_target(${target} result)
    endif()

    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

function(qt_internal_sbom_get_external_document_ref_spdx_id project_name out_var)
    set(result "")

    if(QT_GENERATE_SBOM)
        string(TOLOWER "${project_name}" project_name_lowercase)
        _qt_internal_sbom_get_external_document_ref_spdx_id("${project_name_lowercase}" result)
    endif()

    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

macro(qt_internal_sbom_get_git_version_vars)
    _qt_internal_sbom_get_git_version_vars()
endmacro()

function(qt_internal_sbom_get_project_supplier out_var)
    get_property(result GLOBAL PROPERTY _qt_sbom_project_supplier)
    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

function(qt_internal_sbom_get_project_supplier_url out_var)
    get_property(result GLOBAL PROPERTY _qt_sbom_project_supplier_url)
    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

function(qt_internal_sbom_get_project_namespace out_var)
    get_property(result GLOBAL PROPERTY _qt_sbom_project_namespace)
    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

function(qt_internal_sbom_compute_project_namespace out_var)
    _qt_internal_sbom_compute_project_namespace(result ${ARGN})
    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

function(qt_internal_sbom_compute_project_file_name out_var)
    _qt_internal_sbom_compute_project_file_name(result ${ARGN})
    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

function(qt_internal_sbom_get_sanitized_spdx_id out_var hint)
    _qt_internal_sbom_get_sanitized_spdx_id(result "${hint}")
    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

# Gets a list of default sbom args to use when processing qt entity types.
function(qt_internal_sbom_get_default_sbom_args target out_var)
    _qt_internal_get_sbom_add_target_options(opt_args single_args multi_args)
    list(APPEND opt_args IMMEDIATE_FINALIZATION)
    cmake_parse_arguments(PARSE_ARGV 2 arg "${opt_args}" "${single_args}" "${multi_args}")

    _qt_internal_validate_all_args_are_parsed(arg)

    set(sbom_args "")

    list(APPEND sbom_args USE_ATTRIBUTION_FILES)
    list(APPEND sbom_args __QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_PACKAGE_VERSION)
    list(APPEND sbom_args __QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_SUPPLIER)
    list(APPEND sbom_args __QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_DOWNLOAD_LOCATION)
    list(APPEND sbom_args __QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_LICENSE)
    list(APPEND sbom_args __QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_COPYRIGHTS)
    list(APPEND sbom_args __QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_CPE)
    list(APPEND sbom_args __QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_PURL)
    list(APPEND sbom_args __QT_INTERNAL_HANDLE_QT_ENTITY_ATTRIBUTION_FILES)

    set(${out_var} "${sbom_args}" PARENT_SCOPE)
endfunction()

function(qt_internal_extend_qt_entity_sbom target)
    qt_internal_sbom_get_default_sbom_args("${target}" sbom_extra_args ${ARGN})
    _qt_internal_extend_sbom(${target} ${ARGN} ${sbom_extra_args})
endfunction()
