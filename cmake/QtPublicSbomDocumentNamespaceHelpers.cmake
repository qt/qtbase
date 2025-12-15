# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

function(_qt_internal_sbom_compute_project_namespace out_var)
    set(opt_args "")
    set(single_args
        SUPPLIER_URL
        PROJECT_NAME
        VERSION_SUFFIX
        DOCUMENT_NAMESPACE_INFIX
        DOCUMENT_NAMESPACE_SUFFIX
        DOCUMENT_NAMESPACE_URL_PREFIX
    )
    set(multi_args "")

    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_PROJECT_NAME)
        message(FATAL_ERROR "PROJECT_NAME must be set")
    endif()

    if(NOT arg_SUPPLIER_URL)
        message(FATAL_ERROR "SUPPLIER_URL must be set")
    endif()

    string(TOLOWER "${arg_PROJECT_NAME}" project_name_lowercase)

    set(version_suffix "")

    if(arg_VERSION_SUFFIX)
        set(version_suffix "-${arg_VERSION_SUFFIX}")
    else()
        _qt_internal_sbom_get_git_version_vars()
        if(QT_SBOM_GIT_VERSION)
            set(version_suffix "-${QT_SBOM_GIT_VERSION}")
        endif()
    endif()

    # Used in external refs, it should be either aa URI + UUID or a URI + checksum.
    # We currently use a URI + git version, which is probably not conformant to the spec.
    set(namespace "${project_name_lowercase}${version_suffix}")

    if(arg_DOCUMENT_NAMESPACE_INFIX)
        string(APPEND namespace "${arg_DOCUMENT_NAMESPACE_INFIX}")
    endif()

    if(arg_DOCUMENT_NAMESPACE_SUFFIX)
        string(APPEND namespace "${arg_DOCUMENT_NAMESPACE_SUFFIX}")
    endif()

    if(arg_DOCUMENT_NAMESPACE_URL_PREFIX)
        set(url_prefix "${arg_DOCUMENT_NAMESPACE_URL_PREFIX}")
    else()
        set(url_prefix "${arg_SUPPLIER_URL}/spdxdocs")
    endif()

    set(repo_spdx_namespace "${url_prefix}/${namespace}")

    set(${out_var} "${repo_spdx_namespace}" PARENT_SCOPE)
endfunction()
