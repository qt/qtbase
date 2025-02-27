# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Starts repo sbom generation.
# Should be called before any targets are added to the sbom.
#
# INSTALL_PREFIX should be passed a value like CMAKE_INSTALL_PREFIX or QT_STAGING_PREFIX.
# The default value is \${CMAKE_INSTALL_PREFIX}, which is evaluated at install time, not configure
# time.
# This default value is the /preferred/ value, to ensure using cmake --install . --prefix <path>
# works correctly for lookup of installed files during SBOM generation.
#
# INSTALL_SBOM_DIR should be passed a value like CMAKE_INSTALL_DATAROOTDIR or
#   Qt's INSTALL_SBOMDIR.
# The default value is "sbom".
#
# SUPPLIER, SUPPLIER_URL, DOCUMENT_NAMESPACE, COPYRIGHTS are self-explanatory.
function(_qt_internal_sbom_begin_project)
    # Allow opt out via an internal variable. Will be used in CI for repos like qtqa.
    if(QT_INTERNAL_FORCE_NO_GENERATE_SBOM)
        set(QT_GENERATE_SBOM OFF CACHE BOOL "Generate SBOM" FORCE)
    endif()

    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    _qt_internal_sbom_setup_fake_deterministic_build()

    set(opt_args
        USE_GIT_VERSION
        __QT_INTERNAL_HANDLE_QT_REPO
    )
    set(single_args
        INSTALL_PREFIX
        INSTALL_SBOM_DIR
        LICENSE_EXPRESSION
        SUPPLIER
        SUPPLIER_URL
        DOWNLOAD_LOCATION
        DOCUMENT_NAMESPACE
        VERSION
        SBOM_PROJECT_NAME
        QT_REPO_PROJECT_NAME
        CPE
    )
    set(multi_args
        COPYRIGHTS
    )

    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(CMAKE_VERSION LESS_EQUAL "3.19")
        if(QT_IGNORE_MIN_CMAKE_VERSION_FOR_SBOM)
            message(STATUS
                "Using CMake version older than 3.19, and QT_IGNORE_MIN_CMAKE_VERSION_FOR_SBOM was "
                "set to ON. qt_attribution.json files will not be processed.")
        else()
            message(FATAL_ERROR
                "Generating an SBOM requires CMake version 3.19 or newer. You can pass "
                "-DQT_IGNORE_MIN_CMAKE_VERSION_FOR_SBOM=ON to try to generate the SBOM anyway, "
                "but it is not officially supported, and the SBOM might be incomplete.")
        endif()
    endif()

    # The ntia-conformance-checker insists that a SPDX document contain at least one
    # relationship that DESCRIBES a package, and that the package contains the string
    # "Package-" in the spdx id. boot2qt spdx seems to contain the same.

    if(arg_SBOM_PROJECT_NAME)
        _qt_internal_sbom_set_root_project_name("${arg_SBOM_PROJECT_NAME}")
    else()
        _qt_internal_sbom_set_root_project_name("${PROJECT_NAME}")
    endif()

    if(arg_QT_REPO_PROJECT_NAME)
        _qt_internal_sbom_set_qt_repo_project_name("${arg_QT_REPO_PROJECT_NAME}")
    else()
        _qt_internal_sbom_set_qt_repo_project_name("${PROJECT_NAME}")
    endif()

    _qt_internal_sbom_get_root_project_name_for_spdx_id(repo_project_name_for_spdx_id)
    _qt_internal_sbom_get_root_project_name_lower_case(repo_project_name_lowercase)

    set(repo_supplier_url "")
    if(arg_SUPPLIER_URL)
        set(repo_supplier_url "${arg_SUPPLIER_URL}")
    elseif(arg___QT_INTERNAL_HANDLE_QT_REPO)
        _qt_internal_sbom_get_default_supplier_url(repo_supplier_url)
    endif()

    # Manual override.
    if(arg_VERSION)
        set(QT_SBOM_GIT_VERSION "${arg_VERSION}")
        set(QT_SBOM_GIT_VERSION_PATH "${arg_VERSION}")
        set(QT_SBOM_GIT_HASH "") # empty on purpose, no source of info
        set(QT_SBOM_GIT_HASH_SHORT "") # empty on purpose, no source of info
        set(non_git_version "${arg_VERSION}")
    elseif(arg_USE_GIT_VERSION)
        # Query git version info.
        _qt_internal_find_git_package()
        _qt_internal_query_git_version(
            EMPTY_VALUE_WHEN_NOT_GIT_REPO
            OUT_VAR_PREFIX __sbom_
        )
        set(QT_SBOM_GIT_VERSION "${__sbom_git_version}")
        set(QT_SBOM_GIT_VERSION_PATH "${__sbom_git_version_path}")
        set(QT_SBOM_GIT_HASH "${__sbom_git_hash}")
        set(QT_SBOM_GIT_HASH_SHORT "${__sbom_git_hash_short}")

        # Git version might not be available.
        set(non_git_version "${QT_REPO_MODULE_VERSION}")
        if(NOT QT_SBOM_GIT_VERSION)
            set(QT_SBOM_GIT_VERSION "${non_git_version}")
        endif()
        if(NOT QT_SBOM_GIT_VERSION_PATH)
            set(QT_SBOM_GIT_VERSION_PATH "${non_git_version}")
        endif()
    endif()

    # Save the variables in a global property to later query them in other functions.
    set_property(GLOBAL PROPERTY QT_SBOM_GIT_VERSION "${QT_SBOM_GIT_VERSION}")
    set_property(GLOBAL PROPERTY QT_SBOM_GIT_VERSION_PATH "${QT_SBOM_GIT_VERSION_PATH}")
    set_property(GLOBAL PROPERTY QT_SBOM_GIT_HASH "${QT_SBOM_GIT_HASH}")
    set_property(GLOBAL PROPERTY QT_SBOM_GIT_HASH_SHORT "${QT_SBOM_GIT_HASH_SHORT}")

    if(arg_DOCUMENT_NAMESPACE)
        set(repo_spdx_namespace "${arg_DOCUMENT_NAMESPACE}")
    else()
        _qt_internal_sbom_compute_project_namespace(repo_spdx_namespace
            PROJECT_NAME "${repo_project_name_lowercase}"
            SUPPLIER_URL "${repo_supplier_url}"
        )
    endif()

    if(arg_INSTALL_SBOM_DIR)
        set(install_sbom_dir "${arg_INSTALL_SBOM_DIR}")
    elseif(INSTALL_SBOMDIR)
        set(install_sbom_dir "${INSTALL_SBOMDIR}")
    else()
        set(install_sbom_dir "sbom")
    endif()

    if(arg_INSTALL_PREFIX)
        set(install_prefix "${arg_INSTALL_PREFIX}")
    else()
        # The variable is escaped, so it is evaluated during cmake install time, so that the value
        # can be overridden with cmake --install . --prefix <path>.
        set(install_prefix "\${CMAKE_INSTALL_PREFIX}")
    endif()

    _qt_internal_sbom_compute_project_file_name(repo_project_file_name
        PROJECT_NAME "${repo_project_name_lowercase}"
        VERSION_SUFFIX "${non_git_version}"
    )

    set(repo_spdx_relative_install_path
        "${arg_INSTALL_SBOM_DIR}/${repo_project_file_name}")

    # Prepend DESTDIR, to allow relocating installed sbom. Needed for CI.
    set(repo_spdx_install_path
        "\$ENV{DESTDIR}${install_prefix}/${repo_spdx_relative_install_path}")

    if(arg_LICENSE_EXPRESSION)
        set(repo_license "${arg_LICENSE_EXPRESSION}")
    else()
        # Default to NOASSERTION for root repo SPDX packages, because we have some repos
        # with multiple licenses and AND-ing them together will create a giant unreadable list.
        # It's better to rely on the more granular package licenses.
        set(repo_license "")
    endif()

    set(repo_license_option "")
    if(repo_license)
        set(repo_license_option "LICENSE" "${repo_license}")
    endif()

    if(arg_COPYRIGHTS)
        list(JOIN arg_COPYRIGHTS "\n" arg_COPYRIGHTS)
        set(repo_copyright "<text>${arg_COPYRIGHTS}</text>")
    elseif(arg___QT_INTERNAL_HANDLE_QT_REPO)
        _qt_internal_sbom_get_default_qt_copyright_header(repo_copyright)
    endif()

    if(arg_SUPPLIER)
        set(repo_supplier "${arg_SUPPLIER}")
    elseif(arg___QT_INTERNAL_HANDLE_QT_REPO)
        _qt_internal_sbom_get_default_supplier(repo_supplier)
    endif()

    if(arg_CPE)
        set(qt_cpe "${arg_CPE}")
    elseif(arg___QT_INTERNAL_HANDLE_QT_REPO)
        _qt_internal_sbom_get_cpe_qt_repo(qt_cpe)
    else()
        set(qt_cpe "")
    endif()

    if(arg_DOWNLOAD_LOCATION)
        set(download_location "${arg_DOWNLOAD_LOCATION}")
    elseif(arg___QT_INTERNAL_HANDLE_QT_REPO)
        _qt_internal_sbom_get_qt_repo_source_download_location(download_location)
    endif()

    set(project_comment "")

    if(arg___QT_INTERNAL_HANDLE_QT_REPO)
        _qt_internal_get_configure_line(configure_line)
        if(configure_line)
            set(configure_line_comment
                "\n${repo_project_name_lowercase} was configured with:\n    ${configure_line}\n")
            string(APPEND project_comment "${configure_line_comment}")
        endif()
    endif()

    _qt_internal_sbom_begin_project_generate(
        OUTPUT "${repo_spdx_install_path}"
        OUTPUT_RELATIVE_PATH "${repo_spdx_relative_install_path}"
        ${repo_license_option}
        COPYRIGHT "${repo_copyright}"
        SUPPLIER "${repo_supplier}" # This must not contain spaces!
        SUPPLIER_URL "${repo_supplier_url}"
        DOWNLOAD_LOCATION "${download_location}"
        PROJECT "${repo_project_name_lowercase}"
        PROJECT_COMMENT "${project_comment}"
        PROJECT_FOR_SPDX_ID "${repo_project_name_for_spdx_id}"
        NAMESPACE "${repo_spdx_namespace}"
        CPE "${qt_cpe}"
        OUT_VAR_PROJECT_SPDX_ID repo_project_spdx_id
    )

    set_property(GLOBAL PROPERTY _qt_internal_project_attribution_files "")

    set_property(GLOBAL PROPERTY _qt_internal_sbom_repo_document_namespace
        "${repo_spdx_namespace}")

    set_property(GLOBAL PROPERTY _qt_internal_sbom_relative_installed_repo_document_path
        "${repo_spdx_relative_install_path}")

    set_property(GLOBAL PROPERTY _qt_internal_sbom_repo_project_name_lowercase
        "${repo_project_name_lowercase}")

    set_property(GLOBAL PROPERTY _qt_internal_sbom_install_prefix
        "${arg_INSTALL_PREFIX}")

    set_property(GLOBAL PROPERTY _qt_internal_sbom_project_spdx_id
        "${repo_project_spdx_id}")

    # Collect project licenses.
    set(license_dirs "")

    if(arg___QT_INTERNAL_HANDLE_QT_REPO AND EXISTS "${PROJECT_SOURCE_DIR}/LICENSES")
        list(APPEND license_dirs "${PROJECT_SOURCE_DIR}/LICENSES")
    endif()

    # Allow specifying extra license dirs via a variable. Useful for standalone projects
    # like sqldrivers.
    if(QT_SBOM_LICENSE_DIRS)
        foreach(license_dir IN LISTS QT_SBOM_LICENSE_DIRS)
            if(EXISTS "${license_dir}")
                list(APPEND license_dirs "${license_dir}")
            endif()
        endforeach()
    endif()
    list(REMOVE_DUPLICATES license_dirs)

    set(license_file_wildcard "LicenseRef-*.txt")
    list(TRANSFORM license_dirs APPEND "/${license_file_wildcard}" OUTPUT_VARIABLE license_globs)

    file(GLOB license_files ${license_globs})

    foreach(license_file IN LISTS license_files)
        get_filename_component(license_id "${license_file}" NAME_WLE)
        _qt_internal_sbom_add_license(
            LICENSE_ID "${license_id}"
            LICENSE_PATH "${license_file}"
            NO_LICENSE_REF_PREFIX
        )
    endforeach()

    # Make sure that any system library dependencies that have been found via qt_find_package or
    # _qt_internal_find_third_party_dependencies have their spdx id registered now.
    _qt_internal_sbom_record_system_library_spdx_ids()

    set_property(GLOBAL PROPERTY _qt_internal_sbom_repo_begin_called TRUE)

    _qt_internal_sbom_setup_project_ops()
endfunction()

# Check various internal options to decide which sbom generation operations should be setup.
# Considered operations are generation of a JSON sbom, validation of the SBOM, NTIA checker, etc.
function(_qt_internal_sbom_setup_project_ops)
    set(options "")

    if(QT_SBOM_GENERATE_JSON OR QT_INTERNAL_SBOM_GENERATE_JSON OR QT_INTERNAL_SBOM_DEFAULT_CHECKS)
        list(APPEND options GENERATE_JSON)
    endif()

    # Tring to generate the JSON might fail if the python dependencies are not available.
    # The user can explicitly request to fail the build if dependencies are not found.
    # error out. For internal options that the CI uses, we always want to fail the build if the
    # deps are not found.
    if(QT_SBOM_REQUIRE_GENERATE_JSON OR QT_INTERNAL_SBOM_GENERATE_JSON
            OR QT_INTERNAL_SBOM_DEFAULT_CHECKS)
        list(APPEND options GENERATE_JSON_REQUIRED)
    endif()

    if(QT_SBOM_VERIFY OR QT_INTERNAL_SBOM_VERIFY OR QT_INTERNAL_SBOM_DEFAULT_CHECKS)
        list(APPEND options VERIFY_SBOM)
    endif()

    # Do the same requirement check for SBOM verification.
    if(QT_SBOM_REQUIRE_VERIFY OR QT_INTERNAL_SBOM_VERIFY OR QT_INTERNAL_SBOM_DEFAULT_CHECKS)
        list(APPEND options VERIFY_SBOM_REQUIRED)
    endif()

    if(QT_INTERNAL_SBOM_VERIFY_NTIA_COMPLIANT OR QT_INTERNAL_SBOM_DEFAULT_CHECKS)
        list(APPEND options VERIFY_NTIA_COMPLIANT)
    endif()

    if(QT_INTERNAL_SBOM_SHOW_TABLE OR QT_INTERNAL_SBOM_DEFAULT_CHECKS)
        list(APPEND options SHOW_TABLE)
    endif()

    if(QT_INTERNAL_SBOM_AUDIT OR QT_INTERNAL_SBOM_AUDIT_NO_ERROR)
        list(APPEND options AUDIT)
    endif()

    if(QT_INTERNAL_SBOM_AUDIT_NO_ERROR)
        list(APPEND options AUDIT_NO_ERROR)
    endif()

    if(QT_GENERATE_SOURCE_SBOM)
        list(APPEND options GENERATE_SOURCE_SBOM)
    endif()

    if(QT_LINT_SOURCE_SBOM)
        list(APPEND options LINT_SOURCE_SBOM)
    endif()

    if(QT_INTERNAL_LINT_SOURCE_SBOM_NO_ERROR)
        list(APPEND options LINT_SOURCE_SBOM_NO_ERROR)
    endif()

    _qt_internal_sbom_setup_project_ops_generation(${options})
endfunction()

# Sets up SBOM generation and verification options.
# By default SBOM generation is disabled.
# By default JSON generation and SBOM verification are enabled by default, if the dependencies
# are present, otherwise they will be silently skipped. Unless the user explicitly requests to
# fail the build if the dependencies are not found.
#
# The QT_GENERATE_SBOM_DEFAULT option can be set by a project to change the default value.
function(_qt_internal_setup_sbom)
    set(opt_args "")
    set(single_args
        GENERATE_SBOM_DEFAULT
    )
    set(multi_args "")

    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    set(default_value "OFF")
    if(NOT "${arg_GENERATE_SBOM_DEFAULT}" STREQUAL "")
        set(default_value "${arg_GENERATE_SBOM_DEFAULT}")
    endif()

    option(QT_GENERATE_SBOM "Generate SBOM documents in SPDX v2.3 tag:value format."
        "${default_value}")

    string(CONCAT help_string
        "Generate SBOM documents in SPDX v2.3 JSON format if required python dependency "
        "spdx-tools is available"
    )

    option(QT_SBOM_GENERATE_JSON
        "${help_string}" ON)
    option(QT_SBOM_REQUIRE_GENERATE_JSON
        "Error out if JSON SBOM generation depdendency is not found." OFF)

    option(QT_SBOM_VERIFY "Verify generated SBOM documents using python spdx-tools package." ON)
    option(QT_SBOM_REQUIRE_VERIFY
        "Error out if SBOM verification dependencies are not found." OFF)
endfunction()

# Ends repo sbom project generation.
# Should be called after all relevant targets are added to the sbom.
# Handles registering sbom info for recorded system libraries and then creates the sbom build
# and install rules.
function(_qt_internal_sbom_end_project)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    # Now that we know which system libraries are linked against because we added all
    # subdirectories, we can add the recorded system libs to the sbom.
    _qt_internal_sbom_add_recorded_system_libraries()

    # Run sbom finalization for targets that had it scheduled, but haven't run yet.
    # This can happen when _qt_internal_sbom_end_project is called within the same
    # subdirectory scope as where the targets are meant to be finalized, but that would be too late
    # and the targets wouldn't be added to the sbom.
    # This would mostly happen in user projects, and not Qt repos, because in Qt repos we afaik
    # never create targets in the root cmakelists (aside from the qtbase Platform targets).
    get_cmake_property(targets _qt_internal_sbom_targets_waiting_for_finalization)
    if(targets)
        foreach(target IN LISTS targets)
            _qt_internal_finalize_sbom("${target}")
        endforeach()
    endif()

    _qt_internal_sbom_end_project_generate()

    # Clean up external document ref properties, because each repo needs to start from scratch
    # in a top-level build.
    get_cmake_property(known_external_documents _qt_known_external_documents)
    set_property(GLOBAL PROPERTY _qt_known_external_documents "")
    foreach(external_document IN LISTS known_external_documents)
        set_property(GLOBAL PROPERTY _qt_known_external_documents_${external_document} "")
    endforeach()

    set_property(GLOBAL PROPERTY _qt_internal_sbom_repo_begin_called FALSE)

    # Add configure-time dependency on project attribution files.
    get_property(attribution_files GLOBAL PROPERTY _qt_internal_project_attribution_files)
    list(REMOVE_DUPLICATES attribution_files)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${attribution_files}")
endfunction()

# Automatically begins sbom generation for a qt git repo unless QT_SKIP_SBOM_AUTO_PROJECT is TRUE.
function(_qt_internal_sbom_auto_begin_qt_repo_project)
    # Allow skipping auto generation of sbom project, in case it needs to be manually adjusted with
    # extra parameters.
    if(QT_SKIP_SBOM_AUTO_PROJECT)
        return()
    endif()

    _qt_internal_sbom_begin_qt_repo_project()
endfunction()

# Sets up sbom generation for a qt git repo or qt-git-repo-sub-project (e.g. qtpdf in qtwebengine).
#
# In the case of a qt-git-repo-sub-project, the function expects the following options:
# - SBOM_PROJECT_NAME (e.g. QtPdf)
# - QT_REPO_PROJECT_NAME (e.g. QtWebEngine)
#
# Expects the following variables to always be set before the function call:
# - QT_STAGING_PREFIX
# - INSTALL_SBOMDIR
function(_qt_internal_sbom_begin_qt_repo_project)
    set(opt_args "")
    set(single_args
        SBOM_PROJECT_NAME
        QT_REPO_PROJECT_NAME
    )
    set(multi_args "")

    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    set(sbom_project_args "")

    _qt_internal_forward_function_args(
        FORWARD_APPEND
        FORWARD_PREFIX arg
        FORWARD_OUT_VAR sbom_project_args
        FORWARD_OPTIONS
            ${opt_args}
        FORWARD_SINGLE
            ${single_args}
        FORWARD_MULTI
            ${multi_args}
    )

    _qt_internal_sbom_begin_project(
        INSTALL_SBOM_DIR "${INSTALL_SBOMDIR}"
        USE_GIT_VERSION
        __QT_INTERNAL_HANDLE_QT_REPO
        ${sbom_project_args}
    )
endfunction()

# Automatically ends sbom generation for a qt git repo unless QT_SKIP_SBOM_AUTO_PROJECT is TRUE.
function(_qt_internal_sbom_auto_end_qt_repo_project)
    # Allow skipping auto generation of sbom project, in case it needs to be manually adjusted with
    # extra parameters.
    if(QT_SKIP_SBOM_AUTO_PROJECT)
        return()
    endif()

    _qt_internal_sbom_end_qt_repo_project()
endfunction()

# Ends sbom generation for a qt git repo or qt-git-repo-sub-project.
function(_qt_internal_sbom_end_qt_repo_project)
    _qt_internal_sbom_end_project()
endfunction()


# Enables a fake deterministic SBOM build, for easier inter-diffs between sbom files. Useful
# for local development.
function(_qt_internal_sbom_setup_fake_deterministic_build)
    if(NOT DEFINED QT_SBOM_FAKE_DETERMINISTIC_BUILD)
        return()
    endif()

    if(QT_SBOM_FAKE_DETERMINISTIC_BUILD)
        set(value "ON")
    elseif()
        set(value "OFF")
    endif()

    set(QT_SBOM_FAKE_GIT_VERSION "${value}" CACHE BOOL "SBOM fake git version")
    set(QT_SBOM_FAKE_TIMESTAMP "${value}" CACHE BOOL "SBOM fake timestamp")
    set(QT_SBOM_FAKE_CHECKSUM "${value}" CACHE BOOL "SBOM fake checksums")
endfunction()

# Helper to get purl entry-specific options.
macro(_qt_internal_get_sbom_purl_parsing_options opt_args single_args multi_args)
    set(${opt_args}
        NO_DEFAULT_QT_PURL
        PURL_USE_PACKAGE_VERSION
    )
    set(${single_args}
        PURL_ID
        PURL_TYPE
        PURL_NAMESPACE
        PURL_NAME
        PURL_VERSION
        PURL_SUBPATH
        PURL_VCS_URL
    )
    set(${multi_args}
        PURL_QUALIFIERS
    )
endmacro()

# Helper to get the purl options that should be recongized by sbom functions like
# _qt_internal_sbom_add_target.
macro(_qt_internal_get_sbom_purl_add_target_options opt_args single_args multi_args)
    set(${opt_args}
        __QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_PURL
    )
    set(${single_args} "")
    set(${multi_args}
        PURLS
        PURL_VALUES
    )
endmacro()

# Helper to get purl options that should be forwarded from _qt_internal_sbom_add_target to
# _qt_internal_sbom_handle_purl_values.
macro(_qt_internal_get_sbom_purl_handling_options opt_args single_args multi_args)
    set(${opt_args} "")
    set(${single_args}
        SUPPLIER
        TYPE
        PACKAGE_VERSION
    )
    set(${multi_args} "")

    _qt_internal_get_sbom_purl_add_target_options(
        purl_add_target_opt_args purl_add_target_single_args purl_add_target_multi_args)
    list(APPEND ${opt_args} ${purl_add_target_opt_args})
    list(APPEND ${single_args} ${purl_add_target_single_args})
    list(APPEND ${multi_args} ${purl_add_target_multi_args})
endmacro()

# Helper to get the options that _qt_internal_sbom_add_target understands, but that are also
# a safe subset for qt_internal_add_module, qt_internal_extend_target, etc to understand.
macro(_qt_internal_get_sbom_add_target_common_options opt_args single_args multi_args)
    set(${opt_args}
        NO_CURRENT_DIR_ATTRIBUTION
        NO_ATTRIBUTION_LICENSE_ID
        NO_DEFAULT_QT_LICENSE
        NO_DEFAULT_QT_LICENSE_ID_LIBRARIES
        NO_DEFAULT_QT_LICENSE_ID_EXECUTABLES
        NO_DEFAULT_DIRECTORY_QT_LICENSE
        NO_DEFAULT_QT_COPYRIGHTS
        NO_DEFAULT_QT_PACKAGE_VERSION
        NO_DEFAULT_QT_SUPPLIER
        SBOM_INCOMPLETE_3RD_PARTY_DEPENDENCIES
        IS_QT_3RD_PARTY_HEADER_MODULE
        USE_ATTRIBUTION_FILES
        CREATE_SBOM_FOR_EACH_ATTRIBUTION
        __QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_PACKAGE_VERSION
        __QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_SUPPLIER
        __QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_DOWNLOAD_LOCATION
        __QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_LICENSE
        __QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_COPYRIGHTS
        __QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_CPE
        __QT_INTERNAL_HANDLE_QT_ENTITY_ATTRIBUTION_FILES
    )
    set(${single_args}
        PACKAGE_VERSION
        FRIENDLY_PACKAGE_NAME
        SUPPLIER
        CPE_VENDOR
        CPE_PRODUCT
        LICENSE_EXPRESSION
        QT_LICENSE_ID
        DOWNLOAD_LOCATION
        ATTRIBUTION_ENTRY_INDEX
        ATTRIBUTION_PARENT_TARGET
        SBOM_PACKAGE_COMMENT
    )
    set(${multi_args}
        COPYRIGHTS
        CPE
        SBOM_DEPENDENCIES
        ATTRIBUTION_FILE_PATHS
        ATTRIBUTION_FILE_DIR_PATHS
        ATTRIBUTION_IDS
        SBOM_RELATIONSHIPS
    )

    _qt_internal_get_sbom_purl_add_target_options(
        purl_add_target_opt_args purl_add_target_single_args purl_add_target_multi_args)
    list(APPEND ${opt_args} ${purl_add_target_opt_args})
    list(APPEND ${single_args} ${purl_add_target_single_args})
    list(APPEND ${multi_args} ${purl_add_target_multi_args})
endmacro()

# Helper to get all known SBOM specific options, without the ones that qt_internal_add_module
# and similar functions understand, like LIBRARIES, INCLUDES, etc.
macro(_qt_internal_get_sbom_specific_options opt_args single_args multi_args)
    set(${opt_args} "")
    set(${single_args} "")
    set(${multi_args} "")

    _qt_internal_get_sbom_add_target_common_options(
        common_opt_args common_single_args common_multi_args)
    list(APPEND ${opt_args} ${common_opt_args})
    list(APPEND ${single_args} ${common_single_args})
    list(APPEND ${multi_args} ${common_multi_args})

    _qt_internal_sbom_get_multi_config_single_args(multi_config_single_args)
    list(APPEND ${single_args} ${multi_config_single_args})
endmacro()

# Helper to get the options that _qt_internal_sbom_add_target understands.
# Also used in qt_find_package_extend_sbom.
macro(_qt_internal_get_sbom_add_target_options opt_args single_args multi_args)
    set(${opt_args}
        NO_INSTALL
    )
    set(${single_args}
        TYPE
    )
    set(${multi_args}
        LIBRARIES
        PUBLIC_LIBRARIES
    )

    _qt_internal_get_sbom_specific_options(
        specific_opt_args specific_single_args specific_multi_args)
    list(APPEND ${opt_args} ${specific_opt_args})
    list(APPEND ${single_args} ${specific_single_args})
    list(APPEND ${multi_args} ${specific_multi_args})
endmacro()

# Generate sbom information for a given target.
# Creates:
# - a SPDX package for the target
# - zero or more SPDX file entries for each installed binary file
# - each binary file entry gets a list of 'generated from source files' section
# - dependency relationships to other target packages
# - other relevant information like licenses, copyright, etc.
# For licenses, copyrights, these can either be passed as options, or read from qt_attribution.json
# files.
# For dependencies, these are either specified via options, or read from properties set on the
# target by qt_internal_extend_target.
function(_qt_internal_sbom_add_target target)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    _qt_internal_get_sbom_add_target_options(opt_args single_args multi_args)
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    get_target_property(target_type ${target} TYPE)

    # Mark the target as a Qt module for sbom processing purposes.
    # Needed for non-standard targets like Bootstrap and QtLibraryInfo, that don't have a Qt::
    # namespace prefix.
    if(arg_TYPE STREQUAL QT_MODULE)
        set_target_properties(${target} PROPERTIES _qt_sbom_is_qt_module TRUE)
    endif()

    set(project_package_options "")

    if(arg_FRIENDLY_PACKAGE_NAME)
        set(package_name_for_spdx_id "${arg_FRIENDLY_PACKAGE_NAME}")
    else()
        set(package_name_for_spdx_id "${target}")
    endif()

    set(package_comment "")

    if(arg_SBOM_INCOMPLETE_3RD_PARTY_DEPENDENCIES)
        string(APPEND package_comment
            "Note: This package was marked as not listing all of its consumed 3rd party "
            "dependencies.\nThus the licensing and copyright information might be incomplete.\n")
    endif()

    if(arg_SBOM_PACKAGE_COMMENT)
        string(APPEND package_comment "${arg_SBOM_PACKAGE_COMMENT}\n")
    endif()

    string(APPEND package_comment "CMake target name: ${target}\n")

    get_target_property(qt_package_name "${target}" _qt_package_name)
    if(qt_package_name)
        get_target_property(qt_module_interface_name "${target}" _qt_module_interface_name)
        set(namespaced_target_name "${QT_CMAKE_EXPORT_NAMESPACE}::${qt_module_interface_name}")

        string(APPEND package_comment
            "CMake exported target name: ${namespaced_target_name}\n")
        string(APPEND package_comment "Contained in CMake package: ${qt_package_name}\n")
    endif()

    # Record the target spdx id right now, so we can refer to it in later attribution targets
    # if needed.
    _qt_internal_sbom_record_target_spdx_id(${target}
        TYPE "${arg_TYPE}"
        PACKAGE_NAME "${package_name_for_spdx_id}"
        OUT_VAR package_spdx_id
    )

    if(arg_USE_ATTRIBUTION_FILES)
        set(attribution_args
            ATTRIBUTION_PARENT_TARGET "${target}"
        )

        # Forward the sbom specific options when handling attribution files because those might
        # create other sbom targets that need to inherit the parent ones.
        _qt_internal_get_sbom_specific_options(sbom_opt_args sbom_single_args sbom_multi_args)

        _qt_internal_forward_function_args(
            FORWARD_APPEND
            FORWARD_PREFIX arg
            FORWARD_OUT_VAR attribution_args
            FORWARD_OPTIONS
                ${sbom_opt_args}
            FORWARD_SINGLE
                ${sbom_single_args}
            FORWARD_MULTI
                ${sbom_multi_args}
        )

        if(NOT arg_NO_CURRENT_DIR_ATTRIBUTION
                AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/qt_attribution.json")
            list(APPEND attribution_args
                ATTRIBUTION_FILE_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/qt_attribution.json"
            )
        endif()

        _qt_internal_sbom_handle_qt_attribution_files(qa ${attribution_args})
    endif()

    # Collect license expressions, but in most cases, each expression needs to be abided, so we
    # AND the accumulated license expressions.
    set(license_expression "")

    if(arg_LICENSE_EXPRESSION)
        set(license_expression "${arg_LICENSE_EXPRESSION}")
    endif()

    if(arg___QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_LICENSE)
        _qt_internal_sbom_forward_sbom_add_target_options(sbom_add_target_args)
        _qt_internal_sbom_handle_qt_entity_license_expression(${target} ${sbom_add_target_args}
            OUT_VAR qt_entity_license_expression)
        if(qt_entity_license_expression)
            _qt_internal_sbom_join_two_license_ids_with_op(
                "${license_expression}" "AND" "${qt_entity_license_expression}"
                license_expression)
        endif()
    endif()

    # Read a license expression from the attribution json file.
    if(arg_USE_ATTRIBUTION_FILES
            AND qa_license_id
            AND NOT arg_NO_ATTRIBUTION_LICENSE_ID)
        if(NOT qa_license_id MATCHES "urn:dje:license")
            _qt_internal_sbom_join_two_license_ids_with_op(
                "${license_expression}" "AND" "${qa_license_id}"
                license_expression)
        else()
            message(DEBUG
                "Attribution license id contains invalid spdx license reference: ${qa_license_id}")
            set(invalid_license_comment
                "    Attribution license ID with invalid spdx license reference: ")
            string(APPEND invalid_license_comment "${qa_license_id}\n")
            string(APPEND package_comment "${invalid_license_comment}")
        endif()
    endif()

    if(license_expression)
        list(APPEND project_package_options LICENSE_CONCLUDED "${license_expression}")
    endif()

    if(license_expression AND
            arg___QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_LICENSE)
        _qt_internal_sbom_forward_sbom_add_target_options(sbom_add_target_args)
        _qt_internal_sbom_handle_qt_entity_license_declared_expression(${target}
            ${sbom_add_target_args}
            LICENSE_CONCLUDED_EXPRESSION "${license_expression}"
            OUT_VAR qt_entity_license_declared_expression)
        if(qt_entity_license_declared_expression)
            list(APPEND project_package_options
                LICENSE_DECLARED "${qt_entity_license_declared_expression}")
        endif()
    endif()

    # Copyrights are additive, so we collect them from all sources that were found.
    set(copyrights "")
    if(arg_COPYRIGHTS)
        list(APPEND copyrights "${arg_COPYRIGHTS}")
    endif()

    if(arg___QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_COPYRIGHTS)
        _qt_internal_sbom_forward_sbom_add_target_options(sbom_add_target_args)
        _qt_internal_sbom_handle_qt_entity_copyrights(${target} ${sbom_add_target_args}
            OUT_VAR qt_copyrights)
        if(qt_copyrights)
            list(APPEND copyrights ${qt_copyrights})
        endif()
    endif()

    if(arg_USE_ATTRIBUTION_FILES AND qa_copyrights)
        list(APPEND copyrights "${qa_copyrights}")
    endif()
    if(copyrights)
        list(JOIN copyrights "\n" copyrights)
        list(APPEND project_package_options COPYRIGHT "<text>${copyrights}</text>")
    endif()

    set(package_version "")
    if(arg_PACKAGE_VERSION)
        set(package_version "${arg_PACKAGE_VERSION}")
    elseif(arg_USE_ATTRIBUTION_FILES AND qa_version)
        set(package_version "${qa_version}")
    endif()

    if(arg___QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_PACKAGE_VERSION)
        _qt_internal_sbom_forward_sbom_add_target_options(sbom_add_target_args)
        _qt_internal_sbom_handle_qt_entity_package_version(${target} ${sbom_add_target_args}
            OUT_VAR qt_entity_package_version)
        if(qt_entity_package_version)
            set(package_version "${qt_entity_package_version}")
        endif()
    endif()

    if(package_version)
        list(APPEND project_package_options VERSION "${package_version}")
    endif()

    set(supplier "")
    if(arg_SUPPLIER)
        set(supplier "${arg_SUPPLIER}")
    endif()

    if(arg___QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_SUPPLIER)
        _qt_internal_sbom_forward_sbom_add_target_options(sbom_add_target_args)
        _qt_internal_sbom_handle_qt_entity_supplier(${target} ${sbom_add_target_args}
            OUT_VAR qt_entity_supplier)
        if(qt_entity_supplier)
            set(supplier "${qt_entity_supplier}")
        endif()
    endif()

    if(supplier)
        list(APPEND project_package_options SUPPLIER "Organization: ${supplier}")
    endif()

    set(download_location "")
    if(arg_DOWNLOAD_LOCATION)
        set(download_location "${arg_DOWNLOAD_LOCATION}")
    endif()

    if(arg___QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_DOWNLOAD_LOCATION)
        _qt_internal_sbom_forward_sbom_add_target_options(sbom_add_target_args)
        _qt_internal_sbom_handle_qt_entity_download_location(${target} ${sbom_add_target_args}
            OUT_VAR qt_entity_download_location)
        if(qt_entity_download_location)
            set(download_location "${qt_entity_download_location}")
        endif()
    endif()

    if(arg_USE_ATTRIBUTION_FILES)
        if(qa_download_location)
            set(download_location "${qa_download_location}")
        elseif(qa_homepage)
            set(download_location "${qa_homepage}")
        endif()
    endif()

    if(arg_TYPE STREQUAL "SYSTEM_LIBRARY")
        # Try to get package url that was set using CMake's set_package_properties function.
        # Relies on querying the internal global property name that CMake sets in its
        # implementation.
        get_cmake_property(target_url _CMAKE_${package_name_for_spdx_id}_URL)
        if(target_url)
            set(download_location "${target_url}")
        endif()
        if(NOT download_location
                AND arg_USE_ATTRIBUTION_FILES
                AND qa_download_location)
            set(download_location "${qa_download_location}")
        endif()
    endif()

    if(download_location)
        list(APPEND project_package_options DOWNLOAD_LOCATION "${download_location}")
    endif()

    _qt_internal_sbom_get_package_purpose("${arg_TYPE}" package_purpose)
    list(APPEND project_package_options PURPOSE "${package_purpose}")

    set(cpe_values "")

    if(arg_CPE)
        list(APPEND cpe_values "${arg_CPE}")
    endif()

    if(arg_CPE_VENDOR AND arg_CPE_PRODUCT)
        _qt_internal_sbom_compute_security_cpe(custom_cpe
            VENDOR "${arg_CPE_VENDOR}"
            PRODUCT "${arg_CPE_PRODUCT}"
            VERSION "${package_version}")
        list(APPEND cpe_values "${custom_cpe}")
    endif()

    if(arg_USE_ATTRIBUTION_FILES AND qa_cpes)
        set(placeholder_args "")
        if(package_version)
            list(APPEND placeholder_args VERSION "${package_version}")
        endif()
        _qt_internal_sbom_replace_qa_placeholders(
            VALUES ${qa_cpes}
            ${placeholder_args}
            OUT_VAR qa_cpes_replaced
        )
        list(APPEND cpe_values "${qa_cpes_replaced}")
    endif()

    if(arg___QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_CPE)
        _qt_internal_sbom_forward_sbom_add_target_options(sbom_add_target_args)
        _qt_internal_sbom_handle_qt_entity_cpe(${target} ${sbom_add_target_args}
            CPE "${cpe_values}"
            OUT_VAR qt_cpe_list)
        if(qt_cpe_list)
            list(APPEND cpe_values ${qt_cpe_list})
        endif()
    endif()

    if(cpe_values)
        list(APPEND project_package_options CPE ${cpe_values})
    endif()

    # Assemble arguments to forward to the function that handles purl options.
    set(purl_args "")
    _qt_internal_get_sbom_purl_add_target_options(purl_opt_args purl_single_args purl_multi_args)
    _qt_internal_forward_function_args(
        FORWARD_APPEND
        FORWARD_PREFIX arg
        FORWARD_OUT_VAR purl_args
        FORWARD_OPTIONS
            ${purl_opt_args}
        FORWARD_SINGLE
            ${purl_single_args}
            TYPE
        FORWARD_MULTI
            ${purl_multi_args}
    )

    if(supplier)
        list(APPEND purl_args SUPPLIER "${supplier}")
    endif()

    if(package_version)
        list(APPEND purl_args PACKAGE_VERSION "${package_version}")
    endif()

    if(arg_USE_ATTRIBUTION_FILES AND qa_purls)
        set(placeholder_args "")
        if(package_version)
            list(APPEND placeholder_args VERSION "${package_version}")
        endif()
        _qt_internal_sbom_replace_qa_placeholders(
            VALUES ${qa_purls}
            ${placeholder_args}
            OUT_VAR qa_purls_replaced
        )

        list(APPEND purl_args PURL_VALUES ${qa_purls_replaced})
    endif()
    list(APPEND purl_args OUT_VAR purl_package_options)

    _qt_internal_sbom_handle_purl_values(${target} ${purl_args})

    if(purl_package_options)
        list(APPEND project_package_options ${purl_package_options})
    endif()

    if(arg_USE_ATTRIBUTION_FILES)
        if(qa_chosen_attribution_file_path)
            _qt_internal_sbom_map_path_to_reproducible_relative_path(relative_attribution_path
                PATH "${qa_chosen_attribution_file_path}"
            )
            string(APPEND package_comment
                "    Information extracted from:\n     ${relative_attribution_path}\n")
        endif()

        if(NOT "${qa_chosen_attribution_entry_index}" STREQUAL "")
            string(APPEND package_comment
                "    Entry index: ${qa_chosen_attribution_entry_index}\n")
        endif()

        if(qa_attribution_id)
            string(APPEND package_comment "    Id: ${qa_attribution_id}\n")
        endif()

        if(qa_attribution_name)
            string(APPEND package_comment "    Name: ${qa_attribution_name}\n")
        endif()

        if(qa_description)
            string(APPEND package_comment "    Description: ${qa_description}\n")
        endif()

        if(qa_qt_usage)
            string(APPEND package_comment "    Qt usage: ${qa_qt_usage}\n")
        endif()

        if(qa_license)
            string(APPEND package_comment "    License: ${qa_license}\n")
        endif()

        if(qa_license_file)
            string(APPEND package_comment "    License file: ${qa_license_file}\n")
        endif()
    endif()

    if(package_comment)
        list(APPEND project_package_options COMMENT "<text>\n${package_comment}</text>")
    endif()

    _qt_internal_sbom_handle_target_dependencies("${target}"
        SPDX_ID "${package_spdx_id}"
        LIBRARIES "${arg_LIBRARIES}"
        PUBLIC_LIBRARIES "${arg_PUBLIC_LIBRARIES}"
        OUT_RELATIONSHIPS relationships
    )

    get_cmake_property(project_spdx_id _qt_internal_sbom_project_spdx_id)
    list(APPEND relationships "${project_spdx_id} CONTAINS ${package_spdx_id}")

    if(arg_SBOM_RELATIONSHIPS)
        list(APPEND relationships "${arg_SBOM_RELATIONSHIPS}")
    endif()

    list(REMOVE_DUPLICATES relationships)
    list(JOIN relationships "\nRelationship: " relationships)
    list(APPEND project_package_options RELATIONSHIP "${relationships}")

    _qt_internal_sbom_generate_add_package(
        PACKAGE "${package_name_for_spdx_id}"
        SPDXID "${package_spdx_id}"
        CONTAINS_FILES
        ${project_package_options}
    )

    set(no_install_option "")
    if(arg_NO_INSTALL)
        set(no_install_option NO_INSTALL)
    endif()

    set(framework_option "")
    if(APPLE AND NOT target_type STREQUAL "INTERFACE_LIBRARY")
        get_target_property(is_framework ${target} FRAMEWORK)
        if(is_framework)
            set(framework_option "FRAMEWORK")
        endif()
    endif()

    set(install_prefix_option "")
    get_cmake_property(install_prefix _qt_internal_sbom_install_prefix)
    if(install_prefix)
        set(install_prefix_option INSTALL_PREFIX "${install_prefix}")
    endif()

    _qt_internal_forward_function_args(
        FORWARD_PREFIX arg
        FORWARD_OUT_VAR target_binary_multi_config_args
        FORWARD_SINGLE
            ${multi_config_single_args}
    )

    set(copyrights_option "")
    if(copyrights)
        set(copyrights_option COPYRIGHTS "${copyrights}")
    endif()

    set(license_option "")
    if(license_expression)
        set(license_option LICENSE_EXPRESSION "${license_expression}")
    endif()

    _qt_internal_sbom_handle_target_binary_files("${target}"
        ${no_install_option}
        ${framework_option}
        ${install_prefix_option}
        TYPE "${arg_TYPE}"
        ${target_binary_multi_config_args}
        SPDX_ID "${package_spdx_id}"
        ${copyrights_option}
        ${license_option}
    )

    _qt_internal_sbom_handle_target_custom_files("${target}"
        ${no_install_option}
        ${install_prefix_option}
        PACKAGE_TYPE "${arg_TYPE}"
        PACKAGE_SPDX_ID "${package_spdx_id}"
        ${copyrights_option}
        ${license_option}
    )
endfunction()

# Helper to add sbom information for a possibly non-existing target.
# This will defer the actual sbom generation until the end of the directory scope, unless
# immediate finalization was requested.
function(_qt_internal_add_sbom target)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    set(opt_args
        IMMEDIATE_FINALIZATION
    )
    set(single_args
        TYPE
        FRIENDLY_PACKAGE_NAME
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    # No validation on purpose, the other options will be validated later.

    set(forward_args ${ARGN})

    # Remove the IMMEDIATE_FINALIZATION from the forwarded args.
    list(REMOVE_ITEM forward_args IMMEDIATE_FINALIZATION)

    # If a target doesn't exist we create it.
    if(NOT TARGET "${target}")
        _qt_internal_create_sbom_target("${target}" ${forward_args})
    endif()

    # Save the passed options.
    _qt_internal_extend_sbom("${target}" ${forward_args})

    # Defer finalization. In case it was already deferred, it will be a no-op.
    # Some targets need immediate finalization, like the PlatformInternal ones, because otherwise
    # they would be finalized after the sbom was already generated.
    set(immediate_finalization "")
    if(arg_IMMEDIATE_FINALIZATION)
        set(immediate_finalization IMMEDIATE_FINALIZATION)
    endif()
    _qt_internal_defer_sbom_finalization("${target}" ${immediate_finalization})
endfunction()

# Helper to add custom sbom information for some kind of dependency that is not backed by an
# existing target.
# Useful for cases like 3rd party dependencies not represented by an already existing imported
# target, or for 3rd party sources that get compiled into a regular Qt target (PCRE sources compiled
# into Bootstrap).
function(_qt_internal_create_sbom_target target)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    set(opt_args "")
    set(single_args
        TYPE
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    # No validation on purpose, the other options will be validated later.

    if(TARGET "${target}")
        message(FATAL_ERROR "The target ${target} already exists.")
    endif()

    add_library("${target}" INTERFACE IMPORTED)
    set_target_properties(${target} PROPERTIES
        _qt_sbom_is_custom_sbom_target "TRUE"
        IMPORTED_GLOBAL TRUE
    )

    if(NOT arg_TYPE)
        message(FATAL_ERROR "No SBOM TYPE option was provided for target: ${target}")
    endif()
endfunction()

# Helper to add additional sbom information for an existing target.
# Just appends the options to the target's sbom args property, which will will be evaluated
# during finalization.
function(_qt_internal_extend_sbom target)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    if(NOT TARGET "${target}")
        message(FATAL_ERROR
            "The target ${target} does not exist, use qt_internal_add_sbom to create "
            "a target first, or call the function on any other exsiting target.")
    endif()

    set(opt_args "")
    set(single_args
        TYPE
        FRIENDLY_PACKAGE_NAME
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    # No validation on purpose, the other options will be validated later.

    # Make sure a spdx id is recorded for the target right now, so it is "known" when handling
    # relationships for other targets, even if the target was not yet finalized.
    if(arg_TYPE)
        # Friendly package name is allowed to be empty.
        set(package_name_option "")
        if(arg_FRIENDLY_PACKAGE_NAME)
            set(package_name_option PACKAGE_NAME "${arg_FRIENDLY_PACKAGE_NAME}")
        endif()

        _qt_internal_sbom_record_target_spdx_id(${target}
            TYPE "${arg_TYPE}"
            ${package_name_option}
        )
    endif()

    set_property(TARGET ${target} APPEND PROPERTY _qt_finalize_sbom_args "${ARGN}")
endfunction()

# Helper to add additional sbom information to targets created by qt_find_package.
# If the package was not found, and the targets were not created, the functions does nothing.
# This is similar to _qt_internal_extend_sbom, but is explicit in the fact that the targets might
# not exist.
function(_qt_find_package_extend_sbom)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    _qt_internal_get_sbom_add_target_options(sbom_opt_args sbom_single_args sbom_multi_args)

    set(opt_args
        ${sbom_opt_args}
    )
    set(single_args
        ${sbom_single_args}
    )
    set(multi_args
        TARGETS
        ${sbom_multi_args}
    )

    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    # Make sure not to forward TARGETS.
    set(sbom_args "")
    _qt_internal_forward_function_args(
        FORWARD_APPEND
        FORWARD_PREFIX arg
        FORWARD_OUT_VAR sbom_args
        FORWARD_OPTIONS
            ${sbom_opt_args}
        FORWARD_SINGLE
            ${sbom_single_args}
        FORWARD_MULTI
            ${sbom_multi_args}
    )

    foreach(target IN LISTS arg_TARGETS)
        if(TARGET "${target}")
            _qt_internal_extend_sbom("${target}" ${sbom_args})
        else()
            message(DEBUG "The target ${target} does not exist, skipping extending the sbom info.")
        endif()
    endforeach()
endfunction()

# Helper to defer adding sbom information for a target, at the end of the directory scope.
function(_qt_internal_defer_sbom_finalization target)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    set(opt_args
        IMMEDIATE_FINALIZATION
    )
    set(single_args "")
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    get_target_property(sbom_finalization_requested ${target} _qt_sbom_finalization_requested)
    if(sbom_finalization_requested)
        # Already requested, nothing to do.
        return()
    endif()
    set_target_properties(${target} PROPERTIES _qt_sbom_finalization_requested TRUE)

    _qt_internal_append_to_cmake_property_without_duplicates(
        _qt_internal_sbom_targets_waiting_for_finalization
        "${target}"
    )

    set(func "_qt_internal_finalize_sbom")

    if(arg_IMMEDIATE_FINALIZATION)
        _qt_internal_finalize_sbom(${target})
    elseif(QT_BUILDING_QT)
        qt_add_list_file_finalizer("${func}" "${target}")
    elseif(CMAKE_VERSION VERSION_GREATER_EQUAL "3.19")
        cmake_language(EVAL CODE "cmake_language(DEFER CALL \"${func}\" \"${target}\")")
    else()
        message(FATAL_ERROR "Defer adding a sbom target requires CMake version 3.19")
    endif()
endfunction()

# Finalizer to add sbom information for the target.
# Expects the target to exist.
function(_qt_internal_finalize_sbom target)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    get_target_property(sbom_finalization_done ${target} _qt_sbom_finalization_done)
    if(sbom_finalization_done)
        # Already done, nothing to do.
        return()
    endif()
    set_target_properties(${target} PROPERTIES _qt_sbom_finalization_done TRUE)

    get_target_property(sbom_args ${target} _qt_finalize_sbom_args)
    if(NOT sbom_args)
        set(sbom_args "")
    endif()
    _qt_internal_sbom_add_target(${target} ${sbom_args})
endfunction()

# Extends the list of targets that are considered dependencies for target.
function(_qt_internal_extend_sbom_dependencies target)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    set(opt_args "")
    set(single_args "")
    set(multi_args
        SBOM_DEPENDENCIES
    )
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT TARGET "${target}")
        message(FATAL_ERROR "The target ${target} does not exist.")
    endif()

    _qt_internal_append_to_target_property_without_duplicates(${target}
        _qt_extend_target_sbom_dependencies "${arg_SBOM_DEPENDENCIES}"
    )
endfunction()

# Sets the sbom project name for the root project.
function(_qt_internal_sbom_set_root_project_name project_name)
    set_property(GLOBAL PROPERTY _qt_internal_sbom_repo_project_name "${project_name}")
endfunction()

# Sets the real qt repo project name for a given project (e.g. set QtWebEngine for project QtPdf).
# This is needed to be able to extract the qt repo dependencies in a top-level build.
function(_qt_internal_sbom_set_qt_repo_project_name project_name)
    set_property(GLOBAL PROPERTY _qt_internal_sbom_qt_repo_project_name "${project_name}")
endfunction()

# Get repo project_name spdx id reference, needs to start with Package- to be NTIA compliant.
function(_qt_internal_sbom_get_root_project_name_for_spdx_id out_var)
    _qt_internal_sbom_get_root_project_name_lower_case(repo_project_name_lowercase)
    set(sbom_repo_project_name "Package-${repo_project_name_lowercase}")
    set(${out_var} "${sbom_repo_project_name}" PARENT_SCOPE)
endfunction()

# Returns the lower case sbom project name.
function(_qt_internal_sbom_get_root_project_name_lower_case out_var)
    get_cmake_property(project_name _qt_internal_sbom_repo_project_name)

    if(NOT project_name)
        message(FATAL_ERROR "No SBOM project name was set.")
    endif()

    string(TOLOWER "${project_name}" repo_project_name_lowercase)
    set(${out_var} "${repo_project_name_lowercase}" PARENT_SCOPE)
endfunction()

# Returns the lower case real qt repo project name (e.g. returns 'qtwebengine' when building the
# project qtpdf).
function(_qt_internal_sbom_get_qt_repo_project_name_lower_case out_var)
    get_cmake_property(project_name _qt_internal_sbom_qt_repo_project_name)

    if(NOT project_name)
        message(FATAL_ERROR "No real Qt repo SBOM project name was set.")
    endif()

    string(TOLOWER "${project_name}" repo_project_name_lowercase)
    set(${out_var} "${repo_project_name_lowercase}" PARENT_SCOPE)
endfunction()

# Get a spdx id to reference an external document.
function(_qt_internal_sbom_get_external_document_ref_spdx_id repo_name out_var)
    set(${out_var} "DocumentRef-${repo_name}" PARENT_SCOPE)
endfunction()

# Sanitize a given value to be used as a SPDX id.
function(_qt_internal_sbom_get_sanitized_spdx_id out_var hint)
    # Only allow alphanumeric characters and dashes.
    string(REGEX REPLACE "[^a-zA-Z0-9]+" "-" spdx_id "${hint}")

    # Remove all trailing dashes.
    string(REGEX REPLACE "-+$" "" spdx_id "${spdx_id}")

    set(${out_var} "${spdx_id}" PARENT_SCOPE)
endfunction()

# Generates a spdx id for a target and saves it its properties.
function(_qt_internal_sbom_record_target_spdx_id target)
    set(opt_args "")
    set(single_args
        PACKAGE_NAME
        TYPE
        OUT_VAR
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    _qt_internal_sbom_get_spdx_id_for_target("${target}" spdx_id)

    if(spdx_id)
        # Return early if the target was already recorded and has a spdx id.
        if(arg_OUT_VAR)
            set(${arg_OUT_VAR} "${spdx_id}" PARENT_SCOPE)
        endif()
        return()
    endif()

    if(arg_PACKAGE_NAME)
        set(package_name_for_spdx_id "${arg_PACKAGE_NAME}")
    else()
        set(package_name_for_spdx_id "${target}")
    endif()

    _qt_internal_sbom_generate_target_package_spdx_id(package_spdx_id
        TYPE "${arg_TYPE}"
        PACKAGE_NAME "${package_name_for_spdx_id}"
    )
    _qt_internal_sbom_save_spdx_id_for_target("${target}" "${package_spdx_id}")

    _qt_internal_sbom_is_qt_entity_type("${arg_TYPE}" is_qt_entity_type)
    _qt_internal_sbom_save_spdx_id_for_qt_entity_type(
        "${target}" "${is_qt_entity_type}" "${package_spdx_id}")

    if(arg_OUT_VAR)
        set(${arg_OUT_VAR} "${package_spdx_id}" PARENT_SCOPE)
    endif()
endfunction()

# Generates a sanitized spdx id for a target (package) of a specific type.
function(_qt_internal_sbom_generate_target_package_spdx_id out_var)
    set(opt_args "")
    set(single_args
        PACKAGE_NAME
        TYPE
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_PACKAGE_NAME)
        message(FATAL_ERROR "PACKAGE_NAME must be set")
    endif()
    if(NOT arg_TYPE)
        message(FATAL_ERROR "TYPE must be set")
    endif()

    _qt_internal_sbom_get_root_project_name_for_spdx_id(repo_project_name_spdx_id)
    _qt_internal_sbom_get_package_infix("${arg_TYPE}" package_infix)

    _qt_internal_sbom_get_sanitized_spdx_id(spdx_id
        "SPDXRef-${repo_project_name_spdx_id}-${package_infix}-${arg_PACKAGE_NAME}")

    set(${out_var} "${spdx_id}" PARENT_SCOPE)
endfunction()

# Save a spdx id for a target inside its target properties.
# Also saves the repo document namespace and relative installed repo document path.
# These are used when generating a SPDX external document reference for exported targets, to
# include them in relationships.
function(_qt_internal_sbom_save_spdx_id_for_target target spdx_id)
    message(DEBUG "Saving spdx id for target ${target}: ${spdx_id}")

    set(target_unaliased "${target}")
    get_target_property(aliased_target "${target}" ALIASED_TARGET)
    if(aliased_target)
        set(target_unaliased ${aliased_target})
    endif()

    set_target_properties(${target_unaliased} PROPERTIES
        _qt_sbom_spdx_id "${spdx_id}")

    # Retrieve repo specific properties.
    get_property(repo_document_namespace
        GLOBAL PROPERTY _qt_internal_sbom_repo_document_namespace)

    get_property(relative_installed_repo_document_path
        GLOBAL PROPERTY _qt_internal_sbom_relative_installed_repo_document_path)

    get_property(project_name_lowercase
        GLOBAL PROPERTY _qt_internal_sbom_repo_project_name_lowercase)

    # And save them on the target.
    set_property(TARGET ${target_unaliased} PROPERTY
        _qt_sbom_spdx_repo_document_namespace
        "${repo_document_namespace}")

    set_property(TARGET ${target_unaliased} PROPERTY
        _qt_sbom_spdx_relative_installed_repo_document_path
        "${relative_installed_repo_document_path}")

    set_property(TARGET ${target_unaliased} PROPERTY
        _qt_sbom_spdx_repo_project_name_lowercase
        "${project_name_lowercase}")

    # Export the properties, so they can be queried by other repos.
    # We also do it for versionless targets.
    set(export_properties
        _qt_sbom_spdx_id
        _qt_sbom_spdx_repo_document_namespace
        _qt_sbom_spdx_relative_installed_repo_document_path
        _qt_sbom_spdx_repo_project_name_lowercase
    )
    set_property(TARGET "${target_unaliased}" APPEND PROPERTY
        EXPORT_PROPERTIES "${export_properties}")
endfunction()

# Returns whether the given sbom type is considered to be a Qt type like a module or a tool.
function(_qt_internal_sbom_is_qt_entity_type sbom_type out_var)
    set(qt_entity_types
        QT_MODULE
        QT_PLUGIN
        QT_APP
        QT_TOOL
        QT_TRANSLATIONS
        QT_RESOURCES
        QT_CUSTOM
        QT_CUSTOM_NO_INFIX
    )

    set(is_qt_entity_type FALSE)
    if(sbom_type IN_LIST qt_entity_types)
        set(is_qt_entity_type TRUE)
    endif()

    set(${out_var} ${is_qt_entity_type} PARENT_SCOPE)
endfunction()

# Returns whether the given sbom type is considered to a Qt 3rd party entity type.
function(_qt_internal_sbom_is_qt_3rd_party_entity_type sbom_type out_var)
    set(entity_types
        QT_THIRD_PARTY_MODULE
        QT_THIRD_PARTY_SOURCES
    )

    set(is_qt_third_party_entity_type FALSE)
    if(sbom_type IN_LIST entity_types)
        set(is_qt_third_party_entity_type TRUE)
    endif()

    set(${out_var} ${is_qt_third_party_entity_type} PARENT_SCOPE)
endfunction()

# Save a spdx id for all known related target names of a given Qt target.
# Related being the namespaced and versionless variants of a Qt target.
# All the related targets will contain the same spdx id.
# So Core, CorePrivate, Qt6::Core, Qt6::CorePrivate, Qt::Core, Qt::CorePrivate will all be
# referred to by the same spdx id.
function(_qt_internal_sbom_save_spdx_id_for_qt_entity_type target is_qt_entity_type package_spdx_id)
    # Assign the spdx id to all known related target names of given the given Qt target.
    set(target_names "")

    if(is_qt_entity_type)
        set(namespaced_target "${QT_CMAKE_EXPORT_NAMESPACE}::${target}")
        set(namespaced_private_target "${QT_CMAKE_EXPORT_NAMESPACE}::${target}Private")
        set(versionless_target "Qt::${target}")
        set(versionless_private_target "Qt::${target}Private")

        list(APPEND target_names
            namespaced_target
            namespaced_private_target
            versionless_target
            versionless_private_target
        )
    endif()

    foreach(target_name IN LISTS ${target_names})
        if(TARGET "${target_name}")
            _qt_internal_sbom_save_spdx_id_for_target("${target_name}" "${package_spdx_id}")
        endif()
    endforeach()
endfunction()

# Retrieves a saved spdx id from the target. Might be empty.
function(_qt_internal_sbom_get_spdx_id_for_target target out_var)
    get_target_property(spdx_id ${target} _qt_sbom_spdx_id)
    set(${out_var} "${spdx_id}" PARENT_SCOPE)
endfunction()

# Returns a package infix for a given target sbom type to be used in spdx package id generation.
function(_qt_internal_sbom_get_package_infix type out_infix)
    if(type STREQUAL "QT_MODULE")
        set(package_infix "qt-module")
    elseif(type STREQUAL "QT_PLUGIN")
        set(package_infix "qt-plugin")
    elseif(type STREQUAL "QML_PLUGIN")
        set(package_infix "qt-qml-plugin") # not used at the moment
    elseif(type STREQUAL "QT_TOOL")
        set(package_infix "qt-tool")
    elseif(type STREQUAL "QT_APP")
        set(package_infix "qt-app")
    elseif(type STREQUAL "QT_THIRD_PARTY_MODULE")
        set(package_infix "qt-bundled-3rdparty-module")
    elseif(type STREQUAL "QT_THIRD_PARTY_SOURCES")
        set(package_infix "qt-3rdparty-sources")
    elseif(type STREQUAL "QT_TRANSLATIONS")
        set(package_infix "qt-translation")
    elseif(type STREQUAL "QT_RESOURCES")
        set(package_infix "qt-resource")
    elseif(type STREQUAL "QT_CUSTOM")
        set(package_infix "qt-custom")
    elseif(type STREQUAL "QT_CUSTOM_NO_INFIX")
        set(package_infix "qt")
    elseif(type STREQUAL "SYSTEM_LIBRARY")
        set(package_infix "system-3rdparty")
    elseif(type STREQUAL "EXECUTABLE")
        set(package_infix "executable")
    elseif(type STREQUAL "LIBRARY")
        set(package_infix "library")
    elseif(type STREQUAL "THIRD_PARTY_LIBRARY")
        set(package_infix "3rdparty-library")
    elseif(type STREQUAL "THIRD_PARTY_LIBRARY_WITH_FILES")
        set(package_infix "3rdparty-library-with-files")
    elseif(type STREQUAL "THIRD_PARTY_SOURCES")
        set(package_infix "3rdparty-sources")
    elseif(type STREQUAL "TRANSLATIONS")
        set(package_infix "translations")
    elseif(type STREQUAL "RESOURCES")
        set(package_infix "resource")
    elseif(type STREQUAL "CUSTOM")
        set(package_infix "custom")
    elseif(type STREQUAL "CUSTOM_NO_INFIX")
        set(package_infix "")
    else()
        message(DEBUG "No package infix due to unknown type: ${type}")
        set(package_infix "")
    endif()
    set(${out_infix} "${package_infix}" PARENT_SCOPE)
endfunction()

# Returns a package purpose for a given target sbom type.
function(_qt_internal_sbom_get_package_purpose type out_purpose)
    if(type STREQUAL "QT_MODULE")
        set(package_purpose "LIBRARY")
    elseif(type STREQUAL "QT_PLUGIN")
        set(package_purpose "LIBRARY")
    elseif(type STREQUAL "QML_PLUGIN")
        set(package_purpose "LIBRARY")
    elseif(type STREQUAL "QT_TOOL")
        set(package_purpose "APPLICATION")
    elseif(type STREQUAL "QT_APP")
        set(package_purpose "APPLICATION")
    elseif(type STREQUAL "QT_THIRD_PARTY_MODULE")
        set(package_purpose "LIBRARY")
    elseif(type STREQUAL "QT_THIRD_PARTY_SOURCES")
        set(package_purpose "LIBRARY")
    elseif(type STREQUAL "QT_TRANSLATIONS")
        set(package_purpose "OTHER")
    elseif(type STREQUAL "QT_RESOURCES")
        set(package_purpose "OTHER")
    elseif(type STREQUAL "QT_CUSTOM")
        set(package_purpose "OTHER")
    elseif(type STREQUAL "QT_CUSTOM_NO_INFIX")
        set(package_purpose "OTHER")
    elseif(type STREQUAL "SYSTEM_LIBRARY")
        set(package_purpose "LIBRARY")
    elseif(type STREQUAL "EXECUTABLE")
        set(package_purpose "APPLICATION")
    elseif(type STREQUAL "LIBRARY")
        set(package_purpose "LIBRARY")
    elseif(type STREQUAL "THIRD_PARTY_LIBRARY")
        set(package_purpose "LIBRARY")
    elseif(type STREQUAL "THIRD_PARTY_LIBRARY_WITH_FILES")
        set(package_purpose "LIBRARY")
    elseif(type STREQUAL "THIRD_PARTY_SOURCES")
        set(package_purpose "LIBRARY")
    elseif(type STREQUAL "TRANSLATIONS")
        set(package_purpose "OTHER")
    elseif(type STREQUAL "RESOURCES")
        set(package_purpose "OTHER")
    elseif(type STREQUAL "CUSTOM")
        set(package_purpose "OTHER")
    elseif(type STREQUAL "CUSTOM_NO_INFIX")
        set(package_purpose "OTHER")
    else()
        set(package_purpose "OTHER")
    endif()
    set(${out_purpose} "${package_purpose}" PARENT_SCOPE)
endfunction()

# Queries the current project git version variables and sets them in the parent scope.
function(_qt_internal_sbom_get_git_version_vars)
    get_cmake_property(QT_SBOM_GIT_VERSION QT_SBOM_GIT_VERSION)
    get_cmake_property(QT_SBOM_GIT_VERSION_PATH QT_SBOM_GIT_VERSION_PATH)
    get_cmake_property(QT_SBOM_GIT_HASH QT_SBOM_GIT_HASH)
    get_cmake_property(QT_SBOM_GIT_HASH_SHORT QT_SBOM_GIT_HASH_SHORT)

    set(QT_SBOM_GIT_VERSION "${QT_SBOM_GIT_VERSION}" PARENT_SCOPE)
    set(QT_SBOM_GIT_VERSION_PATH "${QT_SBOM_GIT_VERSION_PATH}" PARENT_SCOPE)
    set(QT_SBOM_GIT_HASH "${QT_SBOM_GIT_HASH}" PARENT_SCOPE)
    set(QT_SBOM_GIT_HASH_SHORT "${QT_SBOM_GIT_HASH_SHORT}" PARENT_SCOPE)
endfunction()

# Returns the configure line used to configure the current repo or top-level build, by reading
# the config.opt file that the configure script writes out.
# Returns an empty string if configure was not called, but CMake was called directly.
# If the build is reconfigured with bare CMake, the config.opt remains untouched, and thus
# the previous contents is returned.
function(_qt_internal_get_configure_line out_var)
    set(content "")

    if(QT_SUPERBUILD OR PROJECT_NAME STREQUAL "QtBase")
        set(configure_script_name "qt6/configure")
    elseif(PROJECT_NAME STREQUAL "QtBase")
        set(configure_script_name "qtbase/configure")
    else()
        _qt_internal_sbom_get_root_project_name_lower_case(repo_project_name_lowercase)
        set(configure_script_name "qt-configure-module <sources>/${repo_project_name_lowercase}")
    endif()

    if(QT_SUPERBUILD)
        set(config_opt_path "${PROJECT_BINARY_DIR}/../config.opt")
    else()
        set(config_opt_path "${PROJECT_BINARY_DIR}/config.opt")
    endif()

    if(NOT EXISTS "${config_opt_path}")
        message(DEBUG "Couldn't find config.opt file in ${config_opt} for argument extraction.")
        set(${out_var} "${content}" PARENT_SCOPE)
        return()
    endif()

    file(STRINGS "${config_opt_path}" args)
    list(JOIN args " " joined_args)

    set(content "${configure_script_name} ${joined_args}")
    string(STRIP "${content}" content)

    set(${out_var} "${content}" PARENT_SCOPE)
endfunction()

function(_qt_internal_sbom_compute_project_namespace out_var)
    set(opt_args "")
    set(single_args
        SUPPLIER_URL
        PROJECT_NAME
        VERSION_SUFFIX
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
    set(repo_name_and_version "${project_name_lowercase}${version_suffix}")
    set(repo_spdx_namespace
        "${arg_SUPPLIER_URL}/spdxdocs/${repo_name_and_version}")

    set(${out_var} "${repo_spdx_namespace}" PARENT_SCOPE)
endfunction()

function(_qt_internal_sbom_compute_project_file_name out_var)
    set(opt_args
        EXTENSION_JSON
    )
    set(single_args
        PROJECT_NAME
        VERSION_SUFFIX
    )
    set(multi_args "")

    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_PROJECT_NAME)
        message(FATAL_ERROR "PROJECT_NAME must be set")
    endif()

    string(TOLOWER "${arg_PROJECT_NAME}" project_name_lowercase)

    set(version_suffix "")

    if(arg_VERSION_SUFFIX)
        set(version_suffix "-${arg_VERSION_SUFFIX}")
    elseif(QT_REPO_MODULE_VERSION)
        set(version_suffix "-${QT_REPO_MODULE_VERSION}")
    endif()

    if(arg_EXTENSION_JSON)
        set(extension "spdx.json")
    else()
        set(extension "spdx")
    endif()

    set(result
        "${project_name_lowercase}${version_suffix}.${extension}")

    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()
