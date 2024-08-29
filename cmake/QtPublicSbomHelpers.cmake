# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Starts repo sbom generation.
# Should be called before any targets are added to the sbom.
#
# INSTALL_PREFIX should be passed a value like CMAKE_INSTALL_PREFIX or QT_STAGING_PREFIX
# INSTALL_SBOM_DIR should be passed a value like CMAKE_INSTALL_DATAROOTDIR or
#   Qt's INSTALL_SBOMDIR
# SUPPLIER, SUPPLIER_URL, DOCUMENT_NAMESPACE, COPYRIGHTS are self-explanatory.
function(_qt_internal_sbom_begin_project)
    # Allow opt out via an internal variable. Will be used in CI for repos like qtqa.
    if(QT_INTERNAL_FORCE_NO_GENERATE_SBOM)
        set(QT_GENERATE_SBOM OFF CACHE BOOL "Generate SBOM" FORCE)
    endif()

    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    set(opt_args
        QT_CPE
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
    _qt_internal_sbom_get_root_project_name_for_spdx_id(repo_project_name_for_spdx_id)
    _qt_internal_sbom_get_root_project_name_lower_case(repo_project_name_lowercase)

    if(arg_SUPPLIER_URL)
        set(repo_supplier_url "${arg_SUPPLIER_URL}")
    else()
        _qt_internal_sbom_get_default_supplier_url(repo_supplier_url)
    endif()

    # Manual override.
    if(arg_VERSION)
        set(QT_SBOM_GIT_VERSION "${arg_VERSION}")
        set(QT_SBOM_GIT_VERSION_PATH "${arg_VERSION}")
        set(QT_SBOM_GIT_HASH "") # empty on purpose, no source of info
        set(QT_SBOM_GIT_HASH_SHORT "") # empty on purpose, no source of info
        set(non_git_version "${arg_VERSION}")
    else()
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

    # Set the variables in the outer scope, so they can be accessed by the generation functions
    # in QtPublicSbomGenerationHelpers.cmake
    set(QT_SBOM_GIT_VERSION "${QT_SBOM_GIT_VERSION}" PARENT_SCOPE)
    set(QT_SBOM_GIT_VERSION_PATH "${QT_SBOM_GIT_VERSION_PATH}" PARENT_SCOPE)
    set(QT_SBOM_GIT_HASH "${QT_SBOM_GIT_HASH}" PARENT_SCOPE)
    set(QT_SBOM_GIT_HASH_SHORT "${QT_SBOM_GIT_HASH_SHORT}" PARENT_SCOPE)

    if(arg_DOCUMENT_NAMESPACE)
        set(repo_spdx_namespace "${arg_DOCUMENT_NAMESPACE}")
    else()
        # Used in external refs, either URI + UUID or URI + checksum. We use git version for now
        # which is probably not conformat to spec.
        set(repo_name_and_version "${repo_project_name_lowercase}-${QT_SBOM_GIT_VERSION}")
        set(repo_spdx_namespace
            "${repo_supplier_url}/spdxdocs/${repo_name_and_version}")
    endif()

    if(non_git_version)
        set(version_suffix "-${non_git_version}")
    else()
        set(version_suffix "")
    endif()

    set(repo_spdx_relative_install_path
        "${arg_INSTALL_SBOM_DIR}/${repo_project_name_lowercase}${version_suffix}.spdx")

    # Prepend DESTDIR, to allow relocating installed sbom. Needed for CI.
    set(repo_spdx_install_path
        "\$ENV{DESTDIR}${arg_INSTALL_PREFIX}/${repo_spdx_relative_install_path}")

    if(arg_LICENSE_EXPRESSION)
        set(repo_license "${arg_LICENSE_EXPRESSION}")
    else()
        # Default to NOASSERTION for root repo SPDX packages, because we have some repos
        # with multiple licenses and AND-ing them together will create a giant unreadable list.
        # It's better to rely on the more granular package licenses.
        set(repo_license "")
    endif()

    if(arg_COPYRIGHTS)
        list(JOIN arg_COPYRIGHTS "\n" arg_COPYRIGHTS)
        set(repo_copyright "<text>${arg_COPYRIGHTS}</text>")
    else()
        _qt_internal_sbom_get_default_qt_copyright_header(repo_copyright)
    endif()

    if(arg_SUPPLIER)
        set(repo_supplier "${arg_SUPPLIER}")
    else()
        _qt_internal_sbom_get_default_supplier(repo_supplier)
    endif()

    if(arg_CPE)
        set(qt_cpe "${arg_CPE}")
    elseif(arg_QT_CPE)
        _qt_internal_sbom_get_cpe_qt_repo(qt_cpe)
    else()
        set(qt_cpe "")
    endif()

    if(arg_DOWNLOAD_LOCATION)
        set(download_location "${arg_DOWNLOAD_LOCATION}")
    else()
        _qt_internal_sbom_get_qt_repo_source_download_location(download_location)
    endif()

    _qt_internal_sbom_begin_project_generate(
        OUTPUT "${repo_spdx_install_path}"
        OUTPUT_RELATIVE_PATH "${repo_spdx_relative_install_path}"
        LICENSE "${repo_license}"
        COPYRIGHT "${repo_copyright}"
        SUPPLIER "${repo_supplier}" # This must not contain spaces!
        SUPPLIER_URL "${repo_supplier_url}"
        DOWNLOAD_LOCATION "${download_location}"
        PROJECT "${repo_project_name_lowercase}"
        PROJECT_FOR_SPDX_ID "${repo_project_name_for_spdx_id}"
        NAMESPACE "${repo_spdx_namespace}"
        CPE "${qt_cpe}"
        OUT_VAR_PROJECT_SPDX_ID repo_project_spdx_id
    )

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

    _qt_internal_get_current_project_sbom_dir(sbom_dir)
    set_property(GLOBAL APPEND PROPERTY _qt_internal_sbom_dirs "${sbom_dir}")

    file(GLOB license_files "${PROJECT_SOURCE_DIR}/LICENSES/LicenseRef-*.txt")
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

    set(end_project_options "")
    if(QT_INTERNAL_SBOM_VERIFY OR QT_INTERNAL_SBOM_DEFAULT_CHECKS)
        list(APPEND end_project_options VERIFY)
    endif()
    if(QT_INTERNAL_SBOM_SHOW_TABLE OR QT_INTERNAL_SBOM_DEFAULT_CHECKS)
        list(APPEND end_project_options SHOW_TABLE)
    endif()
    if(QT_INTERNAL_SBOM_AUDIT OR QT_INTERNAL_SBOM_AUDIT_NO_ERROR)
        list(APPEND end_project_options AUDIT)
    endif()
    if(QT_INTERNAL_SBOM_AUDIT_NO_ERROR)
        list(APPEND end_project_options AUDIT_NO_ERROR)
    endif()
    if(QT_INTERNAL_SBOM_GENERATE_JSON OR QT_INTERNAL_SBOM_DEFAULT_CHECKS)
        list(APPEND end_project_options GENERATE_JSON)
    endif()

    if(QT_GENERATE_SOURCE_SBOM)
        list(APPEND end_project_options GENERATE_SOURCE_SBOM)
    endif()

    if(QT_LINT_SOURCE_SBOM)
        list(APPEND end_project_options LINT_SOURCE_SBOM)
    endif()

    if(QT_INTERNAL_LINT_SOURCE_SBOM_NO_ERROR)
        list(APPEND end_project_options LINT_SOURCE_SBOM_NO_ERROR)
    endif()

    _qt_internal_sbom_end_project_generate(
        ${end_project_options}
    )

    # Clean up external document ref properties, because each repo needs to start from scratch
    # in a top-level build.
    get_cmake_property(known_external_documents _qt_known_external_documents)
    set_property(GLOBAL PROPERTY _qt_known_external_documents "")
    foreach(external_document IN LISTS known_external_documents)
        set_property(GLOBAL PROPERTY _qt_known_external_documents_${external_document} "")
    endforeach()

    set_property(GLOBAL PROPERTY _qt_internal_sbom_repo_begin_called FALSE)
endfunction()

# Helper to get purl parsing options.
macro(_qt_internal_get_sbom_purl_parsing_options opt_args single_args multi_args)
    set(${opt_args}
        NO_PURL
        NO_DEFAULT_QT_PURL
        PURL_USE_PACKAGE_VERSION
    )
    set(${single_args}
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

# Helper to get the purl variant option names that should be recongized by sbom functions like
# _qt_internal_sbom_add_target.
macro(_qt_internal_get_sbom_purl_add_target_options opt_args single_args multi_args)
    set(${opt_args} "")
    set(${single_args} "")
    set(${multi_args}
        PURL_QT_ARGS
        PURL_3RDPARTY_UPSTREAM_ARGS
        PURL_MIRROR_ARGS
        PURL_QT_VALUES
        PURL_3RDPARTY_UPSTREAM_VALUES
        PURL_MIRROR_VALUES
    )
endmacro()

# Helper to get purl options that should be forwarded from _qt_internal_sbom_add_target to
# _qt_internal_sbom_handle_purl_values.
macro(_qt_internal_get_sbom_purl_handling_options opt_args single_args multi_args)
    set(${opt_args}
        IS_QT_ENTITY_TYPE
    )
    set(${single_args}
        SUPPLIER
        TYPE
        VERSION
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
    )
    set(${single_args}
        PACKAGE_VERSION
        FRIENDLY_PACKAGE_NAME
        CPE_VENDOR
        CPE_PRODUCT
        LICENSE_EXPRESSION
        QT_LICENSE_ID
        DOWNLOAD_LOCATION
        ATTRIBUTION_ENTRY_INDEX
        SBOM_PACKAGE_COMMENT
    )
    set(${multi_args}
        COPYRIGHTS
        CPE
        SBOM_DEPENDENCIES
        ATTRIBUTION_FILE_PATHS
        ATTRIBUTION_FILE_DIR_PATHS
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

    _qt_internal_sbom_is_qt_entity_type("${arg_TYPE}" is_qt_entity_type)
    _qt_internal_sbom_is_qt_3rd_party_entity_type("${arg_TYPE}" is_qt_3rd_party_entity_type)

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

    # Record the target spdx id right now, so we can refer to it in later attribution targets
    # if needed.
    _qt_internal_sbom_record_target_spdx_id(${target}
        TYPE "${arg_TYPE}"
        PACKAGE_NAME "${package_name_for_spdx_id}"
        OUT_VAR package_spdx_id
    )

    set(attribution_args
        PARENT_TARGET "${target}"
    )

    if(is_qt_entity_type)
        list(APPEND attribution_args CREATE_SBOM_FOR_EACH_ATTRIBUTION)
    endif()

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

    # Collect license expressions, but in most cases, each expression needs to be abided, so we
    # AND the accumulated license expressions.
    set(license_expression "")

    if(arg_LICENSE_EXPRESSION)
        set(license_expression "${arg_LICENSE_EXPRESSION}")
    endif()

    # For Qt entities, we have some special handling.
    if(is_qt_entity_type AND NOT arg_NO_DEFAULT_QT_LICENSE AND NOT arg_QT_LICENSE_ID)
        if(arg_TYPE STREQUAL "QT_TOOL" OR arg_TYPE STREQUAL "QT_APP")
            if(QT_SBOM_DEFAULT_QT_LICENSE_ID_EXECUTABLES
                    AND NOT arg_NO_DEFAULT_QT_LICENSE_ID_EXECUTABLES)
                # A repo might contain only the "gpl3" license variant as the default for all
                # executables, so allow setting it at the repo level to avoid having to repeat it
                # for each target.
                _qt_internal_sbom_get_spdx_license_expression(
                    "${QT_SBOM_DEFAULT_QT_LICENSE_ID_EXECUTABLES}" qt_license_expression)
            else()
                # For tools and apps, we use the gpl exception variant by default.
                _qt_internal_sbom_get_spdx_license_expression("QT_COMMERCIAL_OR_GPL3_WITH_EXCEPTION"
                    qt_license_expression)
            endif()

        elseif(QT_SBOM_DEFAULT_QT_LICENSE_ID_LIBRARIES
                AND NOT arg_NO_DEFAULT_QT_LICENSE_ID_LIBRARIES)
            # A repo might contain only the "gpl3" license variant as the default for all modules
            # and plugins, so allow setting it at the repo level to avoid having to repeat it
            # for each target.
            _qt_internal_sbom_get_spdx_license_expression(
                "${QT_SBOM_DEFAULT_QT_LICENSE_ID_LIBRARIES}" qt_license_expression)

        else()
            # Otherwise, for modules and plugins we use the default qt license.
            _qt_internal_sbom_get_spdx_license_expression("QT_DEFAULT" qt_license_expression)
        endif()

        _qt_internal_sbom_join_two_license_ids_with_op(
            "${license_expression}" "AND" "${qt_license_expression}"
            license_expression)
    endif()

    # Some Qt entities might request a specific license from the subset that we usually use.
    if(arg_QT_LICENSE_ID)
        _qt_internal_sbom_get_spdx_license_expression("${arg_QT_LICENSE_ID}"
            requested_license_expression)
        _qt_internal_sbom_join_two_license_ids_with_op(
            "${license_expression}" "AND" "${requested_license_expression}"
            license_expression)
    endif()

    # Allow setting a license expression string per directory scope via a variable.
    if(is_qt_entity_type AND QT_SBOM_LICENSE_EXPRESSION AND NOT arg_NO_DEFAULT_DIRECTORY_QT_LICENSE)
        set(qt_license_expression "${QT_SBOM_LICENSE_EXPRESSION}")
        _qt_internal_sbom_join_two_license_ids_with_op(
            "${license_expression}" "AND" "${qt_license_expression}"
            license_expression)
    endif()

    # Read a license expression from the attribution json file.
    if(qa_license_id AND NOT arg_NO_ATTRIBUTION_LICENSE_ID)
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

        # For qt entities we know the license we provide, so we mark it as declared as well.
        if(is_qt_entity_type)
            list(APPEND project_package_options LICENSE_DECLARED "${license_expression}")
        endif()
    endif()

    # Copyrights are additive, so we collect them from all sources that were found.
    set(copyrights "")
    if(arg_COPYRIGHTS)
        list(APPEND copyrights "${arg_COPYRIGHTS}")
    endif()
    if(is_qt_entity_type AND NOT arg_NO_DEFAULT_QT_COPYRIGHTS)
        _qt_internal_sbom_get_default_qt_copyright_header(qt_default_copyright)
        if(qt_default_copyright)
            list(APPEND copyrights "${qt_default_copyright}")
        endif()
    endif()
    if(qa_copyrights)
        list(APPEND copyrights "${qa_copyrights}")
    endif()
    if(copyrights)
        list(JOIN copyrights "\n" copyrights)
        list(APPEND project_package_options COPYRIGHT "<text>${copyrights}</text>")
    endif()

    set(package_version "")
    if(arg_PACKAGE_VERSION)
        set(package_version "${arg_PACKAGE_VERSION}")
    elseif(is_qt_entity_type AND NOT arg_NO_DEFAULT_QT_PACKAGE_VERSION)
        _qt_internal_sbom_get_default_qt_package_version(package_version)
    elseif(qa_version)
        set(package_version "${qa_version}")
    endif()
    if(package_version)
        list(APPEND project_package_options VERSION "${package_version}")
    endif()

    set(supplier "")
    if((is_qt_entity_type OR is_qt_3rd_party_entity_type)
            AND NOT arg_NO_DEFAULT_QT_SUPPLIER)
        _qt_internal_sbom_get_default_supplier(supplier)
    endif()
    if(supplier)
        list(APPEND project_package_options SUPPLIER "Organization: ${supplier}")
    endif()

    set(download_location "")
    if(arg_DOWNLOAD_LOCATION)
        set(download_location "${arg_DOWNLOAD_LOCATION}")
    elseif(is_qt_entity_type)
        _qt_internal_sbom_get_qt_repo_source_download_location(download_location)
    elseif(arg_TYPE STREQUAL "QT_THIRD_PARTY_MODULE" OR arg_TYPE STREQUAL "QT_THIRD_PARTY_SOURCES")
        if(qa_download_location)
            set(download_location "${qa_download_location}")
        elseif(qa_homepage)
            set(download_location "${qa_homepage}")
        endif()
    elseif(arg_TYPE STREQUAL "SYSTEM_LIBRARY")
        # Try to get package url that was set using CMake's set_package_properties function.
        # Relies on querying the internal global property name that CMake sets in its
        # implementation.
        get_cmake_property(target_url _CMAKE_${package_name_for_spdx_id}_URL)
        if(target_url)
            set(download_location "${target_url}")
        endif()
        if(NOT download_location AND qa_download_location)
            set(download_location "${qa_download_location}")
        endif()
    endif()

    if(download_location)
        list(APPEND project_package_options DOWNLOAD_LOCATION "${download_location}")
    endif()

    _qt_internal_sbom_get_package_purpose("${arg_TYPE}" package_purpose)
    list(APPEND project_package_options PURPOSE "${package_purpose}")

    set(cpe_args "")

    if(arg_CPE)
        list(APPEND cpe_args CPE "${arg_CPE}")
    endif()

    if(arg_CPE_VENDOR AND arg_CPE_PRODUCT)
        _qt_internal_sbom_compute_security_cpe(custom_cpe
            VENDOR "${arg_CPE_VENDOR}"
            PRODUCT "${arg_CPE_PRODUCT}"
            VERSION "${package_version}")
        list(APPEND cpe_args CPE "${custom_cpe}")
    endif()

    if(qa_cpes)
        list(APPEND cpe_args CPE "${qa_cpes}")
    endif()

    # Add the qt-specific CPE if the target is a Qt entity type, or if it's a 3rd party entity type
    # without any CPE specified.
    if(is_qt_entity_type OR (is_qt_3rd_party_entity_type AND NOT cpe_args))
        _qt_internal_sbom_compute_security_cpe_for_qt(cpe_list)
        list(APPEND cpe_args CPE "${cpe_list}")
    endif()

    if(cpe_args)
        list(APPEND project_package_options ${cpe_args})
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

    list(APPEND purl_args SUPPLIER "${supplier}")
    list(APPEND purl_args VERSION "${package_version}")
    if(is_qt_entity_type)
        list(APPEND purl_args IS_QT_ENTITY_TYPE)
    endif()
    if(qa_purls)
        list(APPEND purl_args PURL_3RDPARTY_UPSTREAM_VALUES "${qa_purls}")
    endif()
    list(APPEND purl_args OUT_VAR purl_package_options)

    _qt_internal_sbom_handle_purl_values(${target} ${purl_args})

    if(purl_package_options)
        list(APPEND project_package_options ${purl_package_options})
    endif()

    if(is_qt_3rd_party_entity_type
            OR arg_TYPE STREQUAL "SYSTEM_LIBRARY"
            OR arg_TYPE STREQUAL "THIRD_PARTY_LIBRARY"
            OR arg_TYPE STREQUAL "THIRD_PARTY_LIBRARY_WITH_FILES"
        )
        if(qa_attribution_name)
            string(APPEND package_comment "    Name: ${qa_attribution_name}\n")
        endif()

        if(qa_description)
            string(APPEND package_comment "    Description: ${qa_description}\n")
        endif()

        if(qa_qt_usage)
            string(APPEND package_comment "    Qt usage: ${qa_qt_usage}\n")
        endif()

        if(qa_chosen_attribution_file_path)
            string(APPEND package_comment
                "    Information extracted from:\n     ${qa_chosen_attribution_file_path}\n")
        endif()

        if(NOT "${qa_chosen_attribution_entry_index}" STREQUAL "")
            string(APPEND package_comment
                "    Entry index: ${qa_chosen_attribution_entry_index}\n")
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

    _qt_internal_sbom_handle_target_binary_files("${target}"
        ${no_install_option}
        ${framework_option}
        ${install_prefix_option}
        TYPE "${arg_TYPE}"
        ${target_binary_multi_config_args}
        SPDX_ID "${package_spdx_id}"
        COPYRIGHTS "${copyrights}"
        LICENSE_EXPRESSION "${license_expression}"
    )
endfunction()

# Walks a target's direct dependencies and assembles a list of relationships between the packages
# of the target dependencies.
# Currently handles various Qt targets and system libraries.
function(_qt_internal_sbom_handle_target_dependencies target)
    set(opt_args "")
    set(single_args
        SPDX_ID
        OUT_RELATIONSHIPS
    )
    set(multi_args
        LIBRARIES
        PUBLIC_LIBRARIES
    )
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_SPDX_ID)
        message(FATAL_ERROR "SPDX_ID must be set")
    endif()
    set(package_spdx_id "${arg_SPDX_ID}")


    set(libraries "")
    if(arg_LIBRARIES)
        list(APPEND libraries "${arg_LIBRARIES}")
    endif()

    get_target_property(extend_libraries "${target}" _qt_extend_target_libraries)
    if(extend_libraries)
        list(APPEND libraries ${extend_libraries})
    endif()

    get_target_property(target_type ${target} TYPE)
    set(valid_target_types
        EXECUTABLE
        SHARED_LIBRARY
        MODULE_LIBRARY
        STATIC_LIBRARY
        OBJECT_LIBRARY
    )
    if(target_type IN_LIST valid_target_types)
        get_target_property(link_libraries "${target}" LINK_LIBRARIES)
        if(link_libraries)
            list(APPEND libraries ${link_Libraries})
        endif()
    endif()

    set(public_libraries "")
    if(arg_PUBLIC_LIBRARIES)
        list(APPEND public_libraries "${arg_PUBLIC_LIBRARIES}")
    endif()

    get_target_property(extend_public_libraries "${target}" _qt_extend_target_public_libraries)
    if(extend_public_libraries)
        list(APPEND public_libraries ${extend_public_libraries})
    endif()

    set(sbom_dependencies "")
    if(arg_SBOM_DEPENDENCIES)
        list(APPEND sbom_dependencies "${arg_SBOM_DEPENDENCIES}")
    endif()

    get_target_property(extend_sbom_dependencies "${target}" _qt_extend_target_sbom_dependencies)
    if(extend_sbom_dependencies)
        list(APPEND sbom_dependencies ${extend_sbom_dependencies})
    endif()

    list(REMOVE_DUPLICATES libraries)
    list(REMOVE_DUPLICATES public_libraries)
    list(REMOVE_DUPLICATES sbom_dependencies)

    set(all_direct_libraries ${libraries} ${public_libraries} ${sbom_dependencies})
    list(REMOVE_DUPLICATES all_direct_libraries)

    set(spdx_dependencies "")
    set(relationships "")

    # Go through each direct linked lib.
    foreach(direct_lib IN LISTS all_direct_libraries)
        if(NOT TARGET "${direct_lib}")
            continue()
        endif()

        # Some targets are Qt modules, even though they are not prefixed with Qt::, targets
        # like Bootstrap and QtLibraryInfo. We use the property to differentiate them.
        get_target_property(is_marked_as_qt_module "${direct_lib}" _qt_sbom_is_qt_module)

        # Custom sbom targets created by _qt_internal_create_sbom_target are always imported, so we
        # need to differentiate them via this property.
        get_target_property(is_custom_sbom_target "${direct_lib}" _qt_sbom_is_custom_sbom_target)

        if("${direct_lib}" MATCHES "^(Qt::.*)|(${QT_CMAKE_EXPORT_NAMESPACE}::.*)")
            set(is_qt_prefixed TRUE)
        else()
            set(is_qt_prefixed FALSE)
        endif()

        # is_qt_dependency is not strictly only a qt dependency, it applies to custom sbom
        # targets as well. But I'm having a hard time to come up with a better name.
        if(is_marked_as_qt_module OR is_custom_sbom_target OR is_qt_prefixed)
            set(is_qt_dependency TRUE)
        else()
            set(is_qt_dependency FALSE)
        endif()

        # Regular Qt dependency, depend on the relevant package, either within the current
        # document or via an external document.
        if(is_qt_dependency)
            _qt_internal_sbom_is_external_target_dependency("${direct_lib}"
                OUT_VAR is_dependency_in_external_document
            )

            if(is_dependency_in_external_document)
                # External document case.
                _qt_internal_sbom_add_external_target_dependency(
                    "${package_spdx_id}" "${direct_lib}"
                    extra_spdx_dependencies
                    extra_spdx_relationships
                )
                if(extra_spdx_dependencies)
                    list(APPEND spdx_dependencies "${extra_spdx_dependencies}")
                endif()
                if(extra_spdx_relationships)
                    list(APPEND relationships "${extra_spdx_relationships}")
                endif()
            else()
                # Dependency is part of current repo build.
                _qt_internal_sbom_get_spdx_id_for_target("${direct_lib}" dep_spdx_id)
                if(dep_spdx_id)
                    list(APPEND spdx_dependencies "${dep_spdx_id}")
                else()
                    message(DEBUG "Could not add target dependency on ${direct_lib} "
                        "because no spdx id could be found")
                endif()
            endif()
        else()
            # If it's not a Qt dependency, then it's most likely a 3rd party dependency.
            # If we are looking at a FindWrap dependency, we need to depend on either
            # the system or vendored lib, whichever one the FindWrap script points to.
            # If we are looking at a non-Wrap dependency, it's 99% a system lib.
            __qt_internal_walk_libs(
                "${direct_lib}"
                lib_walked_targets
                _discarded_out_var
                "sbom_targets"
                "collect_targets")

            # Detect if we are dealing with a vendored / bundled lib.
            set(bundled_targets_found FALSE)
            if(lib_walked_targets)
                foreach(lib_walked_target IN LISTS lib_walked_targets)
                    get_target_property(is_3rdparty_bundled_lib
                        "${lib_walked_target}" _qt_module_is_3rdparty_library)
                    _qt_internal_sbom_get_spdx_id_for_target("${lib_walked_target}" lib_spdx_id)

                    # Add a dependency on the vendored lib instead of the Wrap target.
                    if(is_3rdparty_bundled_lib AND lib_spdx_id)
                        list(APPEND spdx_dependencies "${lib_spdx_id}")
                        set(bundled_targets_found TRUE)
                    endif()
                endforeach()
            endif()

            # If no bundled libs were found as a result of walking the Wrap lib, we consider this
            # a system lib, and add a dependency on it directly.
            if(NOT bundled_targets_found)
                _qt_internal_sbom_get_spdx_id_for_target("${direct_lib}" lib_spdx_id)
                _qt_internal_sbom_is_external_target_dependency("${direct_lib}"
                    SYSTEM_LIBRARY
                    OUT_VAR is_dependency_in_external_document
                )

                if(lib_spdx_id)
                    if(NOT is_dependency_in_external_document)
                        list(APPEND spdx_dependencies "${lib_spdx_id}")

                        # Mark the system library is used, so that we later generate an sbom for it.
                        _qt_internal_append_to_cmake_property_without_duplicates(
                            _qt_internal_sbom_consumed_system_library_targets
                            "${direct_lib}"
                        )
                    else()
                        # Refer to the package in the external document. This can be the case
                        # in a top-level build, where a system library is reused across repos.
                        _qt_internal_sbom_add_external_target_dependency(
                            "${package_spdx_id}" "${direct_lib}"
                            extra_spdx_dependencies
                            extra_spdx_relationships
                        )
                        if(extra_spdx_dependencies)
                            list(APPEND spdx_dependencies "${extra_spdx_dependencies}")
                        endif()
                        if(extra_spdx_relationships)
                            list(APPEND relationships "${extra_spdx_relationships}")
                        endif()
                    endif()
                else()
                    message(DEBUG "Could not add target dependency on system library ${direct_lib} "
                        "because no spdx id could be found")
                endif()
            endif()
        endif()
    endforeach()

    foreach(dep_spdx_id IN LISTS spdx_dependencies)
        set(relationship
            "${package_spdx_id} DEPENDS_ON ${dep_spdx_id}"
        )
        list(APPEND relationships "${relationship}")
    endforeach()

    set(${arg_OUT_RELATIONSHIPS} "${relationships}" PARENT_SCOPE)
endfunction()

# Checks whether the current target will have its sbom generated into the current repo sbom
# document, or whether it is present in an external sbom document.
function(_qt_internal_sbom_is_external_target_dependency target)
    set(opt_args
        SYSTEM_LIBRARY
    )
    set(single_args
        OUT_VAR
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    get_target_property(is_imported "${target}" IMPORTED)
    get_target_property(is_custom_sbom_target "${target}" _qt_sbom_is_custom_sbom_target)

    _qt_internal_sbom_get_root_project_name_lower_case(current_repo_project_name)
    get_property(target_repo_project_name TARGET ${target}
        PROPERTY _qt_sbom_spdx_repo_project_name_lowercase)

    if(NOT "${target_repo_project_name}" STREQUAL ""
            AND NOT "${target_repo_project_name}" STREQUAL "${current_repo_project_name}")
        set(part_of_other_repo TRUE)
    else()
        set(part_of_other_repo FALSE)
    endif()

    # A target is in an external document if
    # 1) it is imported, and not a custom sbom target, and not a system library
    # 2) it was created as part of another repo in a top-level build
    if((is_imported AND NOT is_custom_sbom_target AND NOT arg_SYSTEM_LIBRARY)
            OR part_of_other_repo)
        set(is_dependency_in_external_document TRUE)
    else()
        set(is_dependency_in_external_document FALSE)
    endif()

    set(${arg_OUT_VAR} "${is_dependency_in_external_document}" PARENT_SCOPE)
endfunction()

# Handles generating an external document reference SDPX element for each target package that is
# located in a different spdx document.
function(_qt_internal_sbom_add_external_target_dependency
        current_package_spdx_id
        target_dep
        out_spdx_dependencies
        out_spdx_relationships
    )
    set(target "${target_dep}")

    _qt_internal_sbom_get_spdx_id_for_target("${target}" dep_spdx_id)

    if(NOT dep_spdx_id)
        message(DEBUG "Could not add external target dependency on ${target} "
            "because no spdx id could be found")
        set(${out_spdx_dependencies} "" PARENT_SCOPE)
        set(${out_spdx_relationships} "" PARENT_SCOPE)
        return()
    endif()

    set(spdx_dependencies "")
    set(spdx_relationships "")

    # Get the external document path and the repo it belongs to for the given target.
    get_property(relative_installed_repo_document_path TARGET ${target}
        PROPERTY _qt_sbom_spdx_relative_installed_repo_document_path)

    get_property(project_name_lowercase TARGET ${target}
        PROPERTY _qt_sbom_spdx_repo_project_name_lowercase)

    if(relative_installed_repo_document_path AND project_name_lowercase)
        _qt_internal_sbom_get_external_document_ref_spdx_id(
            "${project_name_lowercase}" external_document_ref)

        get_cmake_property(known_external_document
            _qt_known_external_documents_${external_document_ref})

        set(relationship
            "${current_package_spdx_id} DEPENDS_ON ${external_document_ref}:${dep_spdx_id}")

        list(APPEND spdx_relationships "${relationship}")

        # Only add a reference to the external document package, if we haven't done so already.
        if(NOT known_external_document)
            set(install_prefixes "")

            get_cmake_property(install_prefix _qt_internal_sbom_install_prefix)
            list(APPEND install_prefixes "${install_prefix}")

            # Add the current sbom build dirs as install prefixes, so that we can use ninja 'sbom'
            # in top-level builds. This is needed because the external references will point
            # to sbom docs in different build dirs, not just one.
            if(QT_SUPERBUILD)
                get_cmake_property(build_sbom_dirs _qt_internal_sbom_dirs)
                if(build_sbom_dirs)
                    foreach(build_sbom_dir IN LISTS build_sbom_dirs)
                        list(APPEND install_prefixes "${build_sbom_dir}")
                    endforeach()
                endif()
            endif()

            set(external_document "${relative_installed_repo_document_path}")

            _qt_internal_sbom_generate_add_external_reference(
                NO_AUTO_RELATIONSHIP
                EXTERNAL "${dep_spdx_id}"
                FILENAME "${external_document}"
                SPDXID "${external_document_ref}"
                INSTALL_PREFIXES ${install_prefixes}
            )

            set_property(GLOBAL PROPERTY
                _qt_known_external_documents_${external_document_ref} TRUE)
            set_property(GLOBAL APPEND PROPERTY
                _qt_known_external_documents "${external_document_ref}")
        endif()
    else()
        message(WARNING "Missing spdx document path for external ref: "
            "package_name_for_spdx_id ${package_name_for_spdx_id} direct_lib ${direct_lib}")
    endif()

    set(${out_spdx_dependencies} "${spdx_dependencies}" PARENT_SCOPE)
    set(${out_spdx_relationships} "${spdx_relationships}" PARENT_SCOPE)
endfunction()

# Handles addition of binary files SPDX entries for a given target.
# Is multi-config aware.
function(_qt_internal_sbom_handle_target_binary_files target)
    set(opt_args
        NO_INSTALL
        FRAMEWORK
    )
    set(single_args
        TYPE
        SPDX_ID
        LICENSE_EXPRESSION
        INSTALL_PREFIX
    )
    set(multi_args
        COPYRIGHTS
    )

    _qt_internal_sbom_get_multi_config_single_args(multi_config_single_args)
    list(APPEND single_args ${multi_config_single_args})

    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(arg_NO_INSTALL)
        message(DEBUG "Skipping sbom target file processing ${target} because NO_INSTALL is set")
        return()
    endif()

    set(supported_types
        QT_MODULE
        QT_PLUGIN
        QT_APP
        QT_TOOL
        QT_THIRD_PARTY_MODULE
        QT_THIRD_PARTY_SOURCES
        SYSTEM_LIBRARY

        # This will be meant for user projects, and are not currently used by Qt's sbom.
        THIRD_PARTY_LIBRARY
        THIRD_PARTY_LIBRARY_WITH_FILES
        EXECUTABLE
        LIBRARY
    )

    if(NOT arg_TYPE IN_LIST supported_types)
        message(FATAL_ERROR "Unsupported target TYPE for SBOM creation: ${arg_TYPE}")
    endif()

    set(types_without_files
        SYSTEM_LIBRARY
        QT_THIRD_PARTY_SOURCES
        THIRD_PARTY_LIBRARY
    )

    get_target_property(target_type ${target} TYPE)

    if(arg_TYPE IN_LIST types_without_files)
        message(DEBUG "Target ${target} has no binary files to reference in the SBOM "
            "because it has the ${arg_TYPE} type.")
        return()
    endif()

    if(target_type STREQUAL "INTERFACE_LIBRARY")
        message(DEBUG "Target ${target} has no binary files to reference in the SBOM "
            "because it is an INTERFACE_LIBRARY.")
        return()
    endif()

    get_target_property(excluded ${target} _qt_internal_excluded_from_default_target)
    if(excluded)
        message(DEBUG "Target ${target} has no binary files to reference in the SBOM "
            "because it was excluded from the default 'all' target.")
        return()
    endif()

    if(NOT arg_SPDX_ID)
        message(FATAL_ERROR "SPDX_ID must be set")
    endif()
    set(package_spdx_id "${arg_SPDX_ID}")

    set(file_common_options "")

    list(APPEND file_common_options PACKAGE_SPDX_ID "${package_spdx_id}")
    list(APPEND file_common_options PACKAGE_TYPE "${arg_TYPE}")

    if(arg_COPYRIGHTS)
        list(APPEND file_common_options COPYRIGHTS "${arg_COPYRIGHTS}")
    endif()

    if(arg_LICENSE_EXPRESSION)
        list(APPEND file_common_options LICENSE_EXPRESSION "${arg_LICENSE_EXPRESSION}")
    endif()

    if(arg_INSTALL_PREFIX)
        list(APPEND file_common_options INSTALL_PREFIX "${arg_INSTALL_PREFIX}")
    endif()

    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)
    if(is_multi_config)
        set(configs ${CMAKE_CONFIGURATION_TYPES})
    else()
        set(configs "${CMAKE_BUILD_TYPE}")
    endif()

    set(path_suffix "$<TARGET_FILE_NAME:${target}>")

    if(arg_FRAMEWORK)
        set(library_path_kind FRAMEWORK_PATH)
    else()
        set(library_path_kind LIBRARY_PATH)
    endif()

    if(arg_TYPE STREQUAL "QT_TOOL"
            OR arg_TYPE STREQUAL "QT_APP"
            OR arg_TYPE STREQUAL "EXECUTABLE")

        set(valid_executable_types
            "EXECUTABLE"
        )
        if(ANDROID)
            list(APPEND valid_executable_types "MODULE_LIBRARY")
        endif()
        if(NOT target_type IN_LIST valid_executable_types)
            message(FATAL_ERROR "Unsupported target type: ${target_type}")
        endif()

        get_target_property(app_is_bundle ${target} MACOSX_BUNDLE)
        if(app_is_bundle)
            _qt_internal_get_executable_bundle_info(bundle "${target}")
            _qt_internal_path_join(path_suffix "${bundle_contents_binary_dir}" "${path_suffix}")
        endif()

        _qt_internal_sbom_handle_multi_config_target_binary_file(${target}
            PATH_KIND RUNTIME_PATH
            PATH_SUFFIX "${path_suffix}"
            OPTIONS ${file_common_options}
        )
    elseif(arg_TYPE STREQUAL "QT_PLUGIN")
        if(NOT (target_type STREQUAL "SHARED_LIBRARY"
                OR target_type STREQUAL "STATIC_LIBRARY"
                OR target_type STREQUAL "MODULE_LIBRARY"))
            message(FATAL_ERROR "Unsupported target type: ${target_type}")
        endif()

        _qt_internal_sbom_handle_multi_config_target_binary_file(${target}
            PATH_KIND INSTALL_PATH
            PATH_SUFFIX "${path_suffix}"
            OPTIONS ${file_common_options}
        )
    elseif(arg_TYPE STREQUAL "QT_MODULE"
            OR arg_TYPE STREQUAL "QT_THIRD_PARTY_MODULE"
            OR arg_TYPE STREQUAL "LIBRARY"
            OR arg_TYPE STREQUAL "THIRD_PARTY_LIBRARY_WITH_FILES"
        )
        if(WIN32 AND target_type STREQUAL "SHARED_LIBRARY")
            _qt_internal_sbom_handle_multi_config_target_binary_file(${target}
                PATH_KIND RUNTIME_PATH
                PATH_SUFFIX "${path_suffix}"
                OPTIONS ${file_common_options}
            )

            _qt_internal_sbom_handle_multi_config_target_binary_file(${target}
                PATH_KIND ARCHIVE_PATH
                PATH_SUFFIX "$<TARGET_LINKER_FILE_NAME:${target}>"
                OPTIONS
                    ${file_common_options}
                    IMPORT_LIBRARY
                    # OPTIONAL because on Windows the import library might not always be present,
                    # because no symbols are exported.
                    OPTIONAL
            )
        elseif(target_type STREQUAL "SHARED_LIBRARY" OR target_type STREQUAL "STATIC_LIBRARY")
            _qt_internal_sbom_handle_multi_config_target_binary_file(${target}
                PATH_KIND "${library_path_kind}"
                PATH_SUFFIX "${path_suffix}"
                OPTIONS ${file_common_options}
            )
        else()
            message(FATAL_ERROR "Unsupported target type: ${target_type}")
        endif()
    endif()
endfunction()

# Add a binary file of a target to the sbom (e.g a shared library or an executable).
# Adds relationships to the SBOM that the binary file was generated from its source files,
# as well as relationship to the owning package.
function(_qt_internal_sbom_add_binary_file target file_path)
    set(opt_args
        OPTIONAL
        IMPORT_LIBRARY
    )
    set(single_args
        PACKAGE_SPDX_ID
        PACKAGE_TYPE
        LICENSE_EXPRESSION
        CONFIG
        INSTALL_PREFIX
    )
    set(multi_args
        COPYRIGHTS
    )
    cmake_parse_arguments(PARSE_ARGV 2 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_PACKAGE_SPDX_ID)
        message(FATAL_ERROR "PACKAGE_SPDX_ID must be set")
    endif()

    set(file_common_options "")

    if(arg_COPYRIGHTS)
        list(JOIN arg_COPYRIGHTS "\n" copyrights)
        list(APPEND file_common_options COPYRIGHT "<text>${copyrights}</text>")
    endif()

    if(arg_LICENSE_EXPRESSION)
        list(APPEND file_common_options LICENSE "${arg_LICENSE_EXPRESSION}")
    endif()

    if(arg_INSTALL_PREFIX)
        list(APPEND file_common_options INSTALL_PREFIX "${arg_INSTALL_PREFIX}")
    endif()

    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)
    if(is_multi_config)
        set(configs ${CMAKE_CONFIGURATION_TYPES})
    else()
        set(configs "${CMAKE_BUILD_TYPE}")
    endif()

    if(is_multi_config)
        set(spdx_id_suffix "${arg_CONFIG}")
        set(config_to_install_option CONFIG ${arg_CONFIG})
    else()
        set(spdx_id_suffix "")
        set(config_to_install_option "")
    endif()

    set(file_infix "")
    if(arg_IMPORT_LIBRARY)
        set(file_infix "-ImportLibrary")
    endif()

    # We kind of have to add the package infix into the file spdx, otherwise we get file system
    # collisions for cases like the qml tool and Qml library, and apparently cmake's file(GENERATE)
    # is case insensitive for file names.
    _qt_internal_sbom_get_package_infix("${arg_PACKAGE_TYPE}" package_infix)

    _qt_internal_sbom_get_file_spdx_id(
        "${package_infix}-${target}-${file_infix}-${spdx_id_suffix}" spdx_id)

    set(optional "")
    if(arg_OPTIONAL)
        set(optional OPTIONAL)
    endif()

    # Add relationship from owning package.
    set(relationships "${arg_PACKAGE_SPDX_ID} CONTAINS ${spdx_id}")

    # Add source file relationships from which the binary file was generated.
    _qt_internal_sbom_add_source_files("${target}" "${spdx_id}" source_relationships)
    if(source_relationships)
        list(APPEND relationships "${source_relationships}")
    endif()

    set(glue "\nRelationship: ")
    # Replace semicolon with $<SEMICOLON> to avoid errors when passing into sbom_add.
    string(REPLACE ";" "$<SEMICOLON>" relationships "${relationships}")

    # Glue the relationships at generation time, because there some source file relationships
    # will be conditional on genexes, and evaluate to an empty value, and we want to discard
    # such relationships.
    set(relationships "$<JOIN:${relationships},${glue}>")
    set(relationship_option RELATIONSHIP "${relationships}")

    # Add the actual binary file to the latest package.
    _qt_internal_sbom_generate_add_file(
        FILENAME "${file_path}"
        FILETYPE BINARY ${optional}
        SPDXID "${spdx_id}"
        ${file_common_options}
        ${config_to_install_option}
        ${relationship_option}
    )
endfunction()

# Adds source file "generated from" relationship comments to the sbom for a given target.
function(_qt_internal_sbom_add_source_files target spdx_id out_relationships)
    get_target_property(sources ${target} SOURCES)
    list(REMOVE_DUPLICATES sources)

    set(relationships "")

    foreach(source IN LISTS sources)
        # Filter out $<TARGET_OBJECTS: genexes>.
        if(source MATCHES "^\\$<TARGET_OBJECTS:.*>$")
            continue()
        endif()

        # Filter out prl files.
        if(source MATCHES "\.prl$")
            continue()
        endif()

        set(source_entry
"${spdx_id} GENERATED_FROM NOASSERTION\nRelationshipComment: ${CMAKE_CURRENT_SOURCE_DIR}/${source}"
        )
        set(source_non_empty "$<BOOL:${source}>")
        # Some sources are conditional on genexes, so we evaluate them.
        set(relationship "$<${source_non_empty}:$<GENEX_EVAL:${source_entry}>>")
        list(APPEND relationships "${relationship}")
    endforeach()

    set(${out_relationships} "${relationships}" PARENT_SCOPE)
endfunction()

# Adds a license id and its text to the sbom.
function(_qt_internal_sbom_add_license)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    set(opt_args
        NO_LICENSE_REF_PREFIX
    )
    set(single_args
        LICENSE_ID
        LICENSE_PATH
        EXTRACTED_TEXT
    )
    set(multi_args
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_LICENSE_ID)
        message(FATAL_ERROR "LICENSE_ID must be set")
    endif()

    if(NOT arg_TEXT AND NOT arg_LICENSE_PATH)
        message(FATAL_ERROR "Either TEXT or LICENSE_PATH must be set")
    endif()

    # Sanitize the content a bit.
    if(arg_TEXT)
        set(text "${arg_TEXT}")
        string(REPLACE ";" "$<SEMICOLON>" text "${text}")
        string(REPLACE "\"" "\\\"" text "${text}")
    else()
        file(READ "${arg_LICENSE_PATH}" text)
        string(REPLACE ";" "$<SEMICOLON>" text "${text}")
        string(REPLACE "\"" "\\\"" text "${text}")
    endif()

    set(license_id "${arg_LICENSE_ID}")
    if(NOT arg_NO_LICENSE_REF_PREFIX)
        set(license_id "LicenseRef-${license_id}")
    endif()

    _qt_internal_sbom_generate_add_license(
        LICENSE_ID "${license_id}"
        EXTRACTED_TEXT "<text>${text}</text>"
    )
endfunction()

# Records information about a system library target, usually due to a qt_find_package call.
# This information is later used to generate packages for the system libraries, but only after
# confirming that the library was used (linked) into any of the Qt targets.
function(_qt_internal_sbom_record_system_library_usage target)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    set(opt_args "")
    set(single_args
        TYPE
        PACKAGE_VERSION
        FRIENDLY_PACKAGE_NAME
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_TYPE)
        message(FATAL_ERROR "TYPE must be set")
    endif()

    # A package might be looked up more than once, make sure to record it once.
    get_property(already_recorded GLOBAL PROPERTY
        _qt_internal_sbom_recorded_system_library_target_${target})

    if(already_recorded)
        return()
    endif()

    set_property(GLOBAL PROPERTY
        _qt_internal_sbom_recorded_system_library_target_${target} TRUE)

    # Defer spdx id creation until _qt_internal_sbom_begin_project is called, so we know the
    # project name. The project name is used in the package infix generation of the system library,
    # but _qt_internal_sbom_record_system_library_usage might be called before sbom generation
    # has started, e.g. during _qt_internal_find_third_party_dependencies.
    set(spdx_options
        ${target}
        TYPE "${arg_TYPE}"
        PACKAGE_NAME "${arg_FRIENDLY_PACKAGE_NAME}"
    )

    get_cmake_property(sbom_repo_begin_called _qt_internal_sbom_repo_begin_called)
    if(sbom_repo_begin_called)
        _qt_internal_sbom_record_system_library_spdx_id(${target} ${spdx_options})
    else()
        set_property(GLOBAL PROPERTY
            _qt_internal_sbom_recorded_system_library_spdx_options_${target} "${spdx_options}")
    endif()

    # Defer sbom info creation until we detect usage of the system library (whether the library is
    # linked into any other target).
    set_property(GLOBAL APPEND PROPERTY
        _qt_internal_sbom_recorded_system_library_targets "${target}")
    set_property(GLOBAL PROPERTY
        _qt_internal_sbom_recorded_system_library_options_${target} "${ARGN}")
endfunction()

# Helper to record spdx ids of all system library targets that were found so far.
function(_qt_internal_sbom_record_system_library_spdx_ids)
    get_property(recorded_targets GLOBAL PROPERTY _qt_internal_sbom_recorded_system_library_targets)

    if(NOT recorded_targets)
        return()
    endif()

    foreach(target IN LISTS recorded_targets)
        get_property(args GLOBAL PROPERTY
            _qt_internal_sbom_recorded_system_library_spdx_options_${target})

        # qt_find_package PROVIDED_TARGETS might refer to non-existent targets in certain cases,
        # like zstd::libzstd_shared for qt_find_package(WrapZSTD), because we are not sure what
        # kind of zstd build was done. Make sure to check if the target exists before recording it.
        if(TARGET "${target}")
            set(target_unaliased "${target}")
            get_target_property(aliased_target "${target}" ALIASED_TARGET)
            if(aliased_target)
                set(target_unaliased ${aliased_target})
            endif()

            _qt_internal_sbom_record_system_library_spdx_id(${target_unaliased} ${args})
        else()
            message(DEBUG
                "Skipping recording system library for SBOM because target does not exist: "
                " ${target}")
        endif()
    endforeach()
endfunction()

# Helper to record the spdx id of a system library target.
function(_qt_internal_sbom_record_system_library_spdx_id target)
    # Save the spdx id before the sbom info is added, so we can refer to it in relationships.
    _qt_internal_sbom_record_target_spdx_id(${ARGN} OUT_VAR package_spdx_id)

    if(NOT package_spdx_id)
        message(FATAL_ERROR "Could not generate spdx id for system library target: ${target}")
    endif()

    set_property(GLOBAL PROPERTY
        _qt_internal_sbom_recorded_system_library_package_${target} "${package_spdx_id}")
endfunction()

# Goes through the list of consumed system libraries (those that were linked in) and creates
# sbom packages for them.
# Uses information from recorded system libraries (calls to qt_find_package).
function(_qt_internal_sbom_add_recorded_system_libraries)
    get_property(recorded_targets GLOBAL PROPERTY _qt_internal_sbom_recorded_system_library_targets)
    get_property(consumed_targets GLOBAL PROPERTY _qt_internal_sbom_consumed_system_library_targets)

    set(unconsumed_targets "${recorded_targets}")
    set(generated_package_names "")

    foreach(target IN LISTS consumed_targets)
        # Some system targets like qtspeech SpeechDispatcher::SpeechDispatcher might be aliased,
        # and we can't set properties on them, so unalias the target name.
        set(target_original "${target}")
        get_target_property(aliased_target "${target}" ALIASED_TARGET)
        if(aliased_target)
            set(target ${aliased_target})
        endif()

        get_property(args GLOBAL PROPERTY
            _qt_internal_sbom_recorded_system_library_options_${target})
        get_property(package_name GLOBAL PROPERTY
            _qt_internal_sbom_recorded_system_library_package_${target})

        set_property(GLOBAL PROPERTY _qt_internal_sbom_recorded_system_library_target_${target} "")
        set_property(GLOBAL PROPERTY _qt_internal_sbom_recorded_system_library_options_${target} "")
        set_property(GLOBAL PROPERTY _qt_internal_sbom_recorded_system_library_package_${target} "")

        # Guard against generating a package multiple times. Can happen when multiple targets belong
        # to the same package.
        if(sbom_generated_${package_name})
            continue()
        endif()

        # Automatic system library sbom recording happens at project root source dir scope, which
        # means it might accidentally pick up a qt_attribution.json file from the project root,
        # that is not intended to be use for system libraries.
        # For now, explicitly disable using the root attribution file.
        list(APPEND args NO_CURRENT_DIR_ATTRIBUTION)

        list(APPEND generated_package_names "${package_name}")
        set(sbom_generated_${package_name} TRUE)

        _qt_internal_extend_sbom(${target} ${args})
        _qt_internal_finalize_sbom(${target})

        list(REMOVE_ITEM unconsumed_targets "${target_original}")
    endforeach()

    message(DEBUG "System libraries that were recorded, but not consumed: ${unconsumed_targets}")
    message(DEBUG "Generated SBOMs for the following system packages: ${generated_package_names}")

    # Clean up, before configuring next repo project.
    set_property(GLOBAL PROPERTY _qt_internal_sbom_consumed_system_library_targets "")
    set_property(GLOBAL PROPERTY _qt_internal_sbom_recorded_system_library_targets "")
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
        _qt_internal_sbom_record_target_spdx_id(${target}
            TYPE "${arg_TYPE}"
            PACKAGE_NAME "${arg_FRIENDLY_PACKAGE_NAME}"
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

# Handles attribution information for a target.
#
# If CREATE_SBOM_FOR_EACH_ATTRIBUTION is set, a separate sbom target is created for each parsed
# attribution entry, and the new targets are added as dependencies to the parent target.
#
# If CREATE_SBOM_FOR_EACH_ATTRIBUTION is not set, the information read from the first attribution
# entry is added directly to the parent target, aka the the values are propagated to the outer
# function scope to be read.. The rest of the attribution entries are created as separate targets
# and added as dependencies, as if the option was passed.
#
# Handles multiple attribution files and entries within a file.
# Attribution files can be specified either via directories and direct file paths.
# If ATTRIBUTION_ENTRY_INDEX is set, only that specific attribution entry will be processed
# from the given attribution file.
function(_qt_internal_sbom_handle_qt_attribution_files out_prefix_outer)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    if(CMAKE_VERSION LESS_EQUAL "3.19")
        message(DEBUG "CMake version is too low, can't parse attribution.json file.")
        return()
    endif()

    set(opt_args
        CREATE_SBOM_FOR_EACH_ATTRIBUTION
    )
    set(single_args
        PARENT_TARGET
    )
    set(multi_args "")

    _qt_internal_get_sbom_specific_options(sbom_opt_args sbom_single_args sbom_multi_args)
    list(APPEND opt_args ${sbom_opt_args})
    list(APPEND single_args ${sbom_single_args})
    list(APPEND multi_args ${sbom_multi_args})

    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    set(attribution_files "")
    set(attribution_file_count 0)

    foreach(attribution_file_path IN LISTS arg_ATTRIBUTION_FILE_PATHS)
        get_filename_component(real_path "${attribution_file_path}" REALPATH)
        list(APPEND attribution_files "${real_path}")
        math(EXPR attribution_file_count "${attribution_file_count} + 1")
    endforeach()

    foreach(attribution_file_dir_path IN LISTS arg_ATTRIBUTION_FILE_DIR_PATHS)
        get_filename_component(real_path
            "${attribution_file_dir_path}/qt_attribution.json" REALPATH)
        list(APPEND attribution_files "${real_path}")
        math(EXPR attribution_file_count "${attribution_file_count} + 1")
    endforeach()

    # If CREATE_SBOM_FOR_EACH_ATTRIBUTION is set, that means the parent target was a qt entity,
    # and not a 3rd party library.
    # In which case we don't want to proagate options like CPE to the child attribution targets,
    # because the CPE is meant for the parent target.
    set(propagate_sbom_options_to_new_attribution_targets TRUE)
    if(arg_CREATE_SBOM_FOR_EACH_ATTRIBUTION)
        set(propagate_sbom_options_to_new_attribution_targets FALSE)
        if(NOT arg_PARENT_TARGET)
            message(FATAL_ERROR "PARENT_TARGET must be set")
        endif()
    endif()

    if(arg_ATTRIBUTION_ENTRY_INDEX AND attribution_file_count GREATER 1)
        message(FATAL_ERROR
            "ATTRIBUTION_ENTRY_INDEX should only be set if a single attribution "
            "file is specified."
        )
    endif()

    set(file_index 0)
    set(first_attribution_processed FALSE)
    foreach(attribution_file_path IN LISTS attribution_files)
        # Set a unique out_prefix that will not overlap when multiple entries are processed.
        set(out_prefix_file "${out_prefix_outer}_${file_index}")

        # Get the number of entries in the attribution file.
        _qt_internal_sbom_read_qt_attribution(${out_prefix_file}
            GET_ATTRIBUTION_ENTRY_COUNT
            OUT_VAR_VALUE attribution_entry_count
            FILE_PATH "${attribution_file_path}"
        )

        # If a specific entry was specified, we will only process it from the file.
        if(NOT "${arg_ATTRIBUTION_ENTRY_INDEX}" STREQUAL "")
            set(entry_index ${arg_ATTRIBUTION_ENTRY_INDEX})
        else()
            set(entry_index 0)
        endif()

        # Go through each entry in the attribution file.
        while("${entry_index}" LESS "${${out_prefix_file}_attribution_entry_count}")
            # If this is the first entry to be processed, or if CREATE_SBOM_FOR_EACH_ATTRIBUTION
            # is not set, we read the attribution file entry directly, and propagate the values
            # to the parent scope.
            if(NOT first_attribution_processed AND NOT arg_CREATE_SBOM_FOR_EACH_ATTRIBUTION)
                # Set a prefix without indices, so that the parent scope add_sbom call can
                # refer to the values directly with the outer prefix, without any index infix.
                set(out_prefix "${out_prefix_outer}")

                _qt_internal_sbom_read_qt_attribution(${out_prefix}
                    GET_DEFAULT_KEYS
                    ENTRY_INDEX "${entry_index}"
                    OUT_VAR_ASSIGNED_VARIABLE_NAMES variable_names
                    FILE_PATH "${attribution_file_path}"
                )

                # Propagate the values to the outer scope.
                foreach(variable_name IN LISTS variable_names)
                    set(${out_prefix}_${variable_name} "${${out_prefix}_${variable_name}}"
                        PARENT_SCOPE)
                endforeach()

                get_filename_component(relative_attribution_file_path
                    "${attribution_file_path}" REALPATH)

                set(${out_prefix}_chosen_attribution_file_path "${relative_attribution_file_path}"
                    PARENT_SCOPE)
                set(${out_prefix}_chosen_attribution_entry_index "${entry_index}"
                    PARENT_SCOPE)

                set(first_attribution_processed TRUE)
                if(NOT "${arg_ATTRIBUTION_ENTRY_INDEX}" STREQUAL "")
                    # We had a specific index to process, so break right after processing it.
                    break()
                endif()
            else()
                # We are processing the second or later entry, or CREATE_SBOM_FOR_EACH_ATTRIBUTION
                # was set. Instead of directly reading all the keys from the attribution file,
                # we get the Id, and create a new sbom target for the entry.
                # That will recursively call this function with a specific attribution file path
                # and index, to process the specific entry.

                set(out_prefix "${out_prefix_outer}_${file_index}_${entry_index}")

                # Get the attribution id.
                _qt_internal_sbom_read_qt_attribution(${out_prefix}
                    GET_KEY
                    KEY Id
                    OUT_VAR_VALUE attribution_id
                    ENTRY_INDEX "${entry_index}"
                    FILE_PATH "${attribution_file_path}"
                )

                # If no Id was retrieved, just add a numeric one, to make the sbom target
                # unique.
                set(attribution_target "${arg_PARENT_TARGET}_Attribution_")
                if(NOT ${out_prefix}_attribution_id)
                    string(APPEND attribution_target "${file_index}_${entry_index}")
                else()
                    string(APPEND attribution_target "${${out_prefix}_attribution_id}")
                endif()

                set(sbom_args "")

                if(propagate_sbom_options_to_new_attribution_targets)
                    # Filter out the attributtion options, they will be passed mnaually
                    # depending on which file and index is currently being processed.
                    _qt_internal_get_sbom_specific_options(
                        sbom_opt_args sbom_single_args sbom_multi_args)
                    list(REMOVE_ITEM sbom_opt_args NO_CURRENT_DIR_ATTRIBUTION)
                    list(REMOVE_ITEM sbom_single_args ATTRIBUTION_ENTRY_INDEX)
                    list(REMOVE_ITEM sbom_multi_args
                        ATTRIBUTION_FILE_PATHS
                        ATTRIBUTION_FILE_DIR_PATHS
                    )

                    # Also filter out the FRIENDLY_PACKAGE_NAME option, otherwise we'd try to
                    # file(GENERATE) multiple times with the same file name, but different content.
                    list(REMOVE_ITEM sbom_single_args FRIENDLY_PACKAGE_NAME)

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
                endif()

                # Create another sbom target with the id as a hint for the target name,
                # the attribution file passed, and make the new target a dependency of the
                # parent one.
                _qt_internal_add_sbom("${attribution_target}"
                    IMMEDIATE_FINALIZATION
                    TYPE QT_THIRD_PARTY_SOURCES
                    ATTRIBUTION_FILE_PATHS "${attribution_file_path}"
                    ATTRIBUTION_ENTRY_INDEX "${entry_index}"
                    NO_CURRENT_DIR_ATTRIBUTION
                    ${sbom_args}
                )

                _qt_internal_extend_sbom_dependencies(${arg_PARENT_TARGET}
                    SBOM_DEPENDENCIES ${attribution_target}
                )
            endif()

            math(EXPR entry_index "${entry_index} + 1")
        endwhile()

        math(EXPR file_index "${file_index} + 1")
    endforeach()
endfunction()

# Helper to parse a qt_attribution.json file and do various operations:
# - GET_DEFAULT_KEYS extracts the license id, copyrights, version, etc.
# - GET_KEY extracts a single given json key's value, as specified with KEY and saved into
#   OUT_VAR_VALUE
# - GET_ATTRIBUTION_ENTRY_COUNT returns the number of entries in the json file, set in
#   OUT_VAR_VALUE
#
# ENTRY_INDEX can be used to specify the array index to select a specific entry in the json file.
#
# Any retrieved value is set in the outer scope.
# The variables are prefixed with ${out_prefix}.
# OUT_VAR_ASSIGNED_VARIABLE_NAMES contains the list of variables set in the parent scope, the
# variables names in this list are not prefixed with ${out_prefix}.
#
# Requires cmake 3.19 for json parsing.
function(_qt_internal_sbom_read_qt_attribution out_prefix)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    if(CMAKE_VERSION LESS_EQUAL "3.19")
        message(DEBUG "CMake version is too low, can't parse attribution.json file.")
        return()
    endif()

    set(opt_args
        GET_DEFAULT_KEYS
        GET_KEY
        GET_ATTRIBUTION_ENTRY_COUNT
    )
    set(single_args
        FILE_PATH
        KEY
        ENTRY_INDEX
        OUT_VAR_VALUE
        OUT_VAR_ASSIGNED_VARIABLE_NAMES
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    set(file_path "${arg_FILE_PATH}")

    if(NOT file_path)
        message(FATAL_ERROR "qt attribution file path not given")
    endif()

    file(READ "${file_path}" contents)
    if(NOT contents)
        message(FATAL_ERROR "qt attribution file is empty: ${file_path}")
    endif()

    if(NOT arg_GET_DEFAULT_KEYS AND NOT arg_GET_KEY AND NOT arg_GET_ATTRIBUTION_ENTRY_COUNT)
        message(FATAL_ERROR
            "No valid operation specified to _qt_internal_sbom_read_qt_attribution call.")
    endif()

    if(arg_GET_KEY)
        if(NOT arg_KEY)
            message(FATAL_ERROR "KEY must be set")
        endif()
        if(NOT arg_OUT_VAR_VALUE)
            message(FATAL_ERROR "OUT_VAR_VALUE must be set")
        endif()
    endif()

    get_filename_component(attribution_file_dir "${file_path}" DIRECTORY)

    # Parse the json file.
    # The first element might be an array, or an object. We need to detect which one.
    # Do that by trying to query index 0 of the potential root array.
    # If the index is found, that means the root is an array, and elem_error is set to NOTFOUND,
    # because there was no error.
    # Otherwise elem_error will be something like 'member '0' not found', and we can assume the
    # root is an object.
    string(JSON first_elem_type ERROR_VARIABLE elem_error TYPE "${contents}" 0)
    if(elem_error STREQUAL "NOTFOUND")
        # Root is an array. The attribution file might contain multiple entries.
        # Pick the first one if no specific index was specified, otherwise use the given index.
        if(NOT "${arg_ENTRY_INDEX}" STREQUAL "")
            set(indices "${arg_ENTRY_INDEX}")
        else()
            set(indices "0")
        endif()
        set(is_array TRUE)
    else()
        # Root is an object, not an array, which means the file has a single entry.
        set(indices "")
        set(is_array FALSE)
    endif()

    set(variable_names "")

    if(arg_GET_KEY)
        _qt_internal_sbom_get_attribution_key(${arg_KEY} ${arg_OUT_VAR_VALUE} ${out_prefix})
    endif()

    if(arg_GET_ATTRIBUTION_ENTRY_COUNT)
        if(NOT arg_OUT_VAR_VALUE)
            message(FATAL_ERROR "OUT_VAR_VALUE must be set")
        endif()

        if(is_array)
            string(JSON attribution_entry_count ERROR_VARIABLE elem_error LENGTH "${contents}")
            # There was an error getting the length of the array, so we assume it's empty.
            if(NOT elem_error STREQUAL "NOTFOUND")
                set(attribution_entry_count 0)
            endif()
        else()
            set(attribution_entry_count 1)
        endif()

        set(${out_prefix}_${arg_OUT_VAR_VALUE} "${attribution_entry_count}" PARENT_SCOPE)
    endif()

    if(arg_GET_DEFAULT_KEYS)
        # Some calls are currently commented out, to save on json parsing time because we don't have
        # a usage for them yet.
        # _qt_internal_sbom_get_attribution_key(License license)
        _qt_internal_sbom_get_attribution_key(LicenseId license_id "${out_prefix}")
        _qt_internal_sbom_get_attribution_key(Version version "${out_prefix}")
        _qt_internal_sbom_get_attribution_key(Homepage homepage "${out_prefix}")
        _qt_internal_sbom_get_attribution_key(Name attribution_name "${out_prefix}")
        _qt_internal_sbom_get_attribution_key(Description description "${out_prefix}")
        _qt_internal_sbom_get_attribution_key(QtUsage qt_usage "${out_prefix}")
        _qt_internal_sbom_get_attribution_key(DownloadLocation download_location "${out_prefix}")
        _qt_internal_sbom_get_attribution_key(Copyright copyrights "${out_prefix}" IS_MULTI_VALUE)
        _qt_internal_sbom_get_attribution_key(CopyrightFile copyright_file "${out_prefix}")
        _qt_internal_sbom_get_attribution_key(PURL purls "${out_prefix}" IS_MULTI_VALUE)
        _qt_internal_sbom_get_attribution_key(CPE cpes "${out_prefix}" IS_MULTI_VALUE)

        # Some attribution files contain a copyright file that contains the actual list of
        # copyrights. Read it and use it.
        set(copyright_file_path "${attribution_file_dir}/${copyright_file}")
        get_filename_component(copyright_file_path "${copyright_file_path}" REALPATH)
        if(NOT copyrights AND copyright_file AND EXISTS "${copyright_file_path}")
            file(READ "${copyright_file_path}" copyright_contents)
            if(copyright_contents)
                set(copyright_contents "${copyright_contents}")
                set(copyrights "${copyright_contents}")
                set(${out_prefix}_copyrights "${copyright_contents}" PARENT_SCOPE)
                list(APPEND variable_names "copyrights")
            endif()
        endif()
    endif()

    if(arg_OUT_VAR_ASSIGNED_VARIABLE_NAMES)
        set(${arg_OUT_VAR_ASSIGNED_VARIABLE_NAMES} "${variable_names}" PARENT_SCOPE)
    endif()
endfunction()

# Extracts a string or an array of strings from a json index path, depending on the extracted value
# type.
#
# Given the 'contents' of the whole json document and the EXTRACTED_VALUE of a json key specified
# by the INDICES path, it tries to determine whether the value is an array, in which case the array
# is converted to a cmake list and assigned to ${out_var} in the parent scope.
# Otherwise the function assumes the EXTRACTED_VALUE was not an array, and just assigns the value
# of EXTRACTED_VALUE to ${out_var}
function(_qt_internal_sbom_handle_attribution_json_array contents)
    set(opt_args "")
    set(single_args
        EXTRACTED_VALUE
        OUT_VAR
    )
    set(multi_args
        INDICES
    )
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    # Write the original value to the parent scope, in case it was not an array.
    set(${arg_OUT_VAR} "${arg_EXTRACTED_VALUE}" PARENT_SCOPE)

    if(NOT arg_EXTRACTED_VALUE)
        return()
    endif()

    string(JSON element_type TYPE "${contents}" ${arg_INDICES})

    if(NOT element_type STREQUAL "ARRAY")
        return()
    endif()

    set(json_array "${arg_EXTRACTED_VALUE}")
    string(JSON array_len LENGTH "${json_array}")

    set(value_list "")

    math(EXPR array_len "${array_len} - 1")
    foreach(index RANGE 0 "${array_len}")
        string(JSON value GET "${json_array}" ${index})
        if(value)
            list(APPEND value_list "${value}")
        endif()
    endforeach()

    if(value_list)
        set(${arg_OUT_VAR} "${value_list}" PARENT_SCOPE)
    endif()
endfunction()

# Escapes various characters in json content, so that the generate cmake code to append the content
# to the spdx document is syntactically valid.
function(_qt_internal_sbom_escape_json_content content out_var)
    # Escape backslashes
    string(REPLACE "\\" "\\\\" escaped_content "${content}")

    # Escape quotes
    string(REPLACE "\"" "\\\"" escaped_content "${escaped_content}")

    set(${out_var} "${escaped_content}" PARENT_SCOPE)
endfunction()

# This macro reads a json key from a qt_attribution.json file, and assigns the escaped value to
# out_var.
# Also appends the name of the out_var to the parent scope 'variable_names' var.
#
# Expects 'contents' and 'indices' to already be set in the calling scope.
#
# If IS_MULTI_VALUE is set, handles the key as if it contained an array of
# values, by converting the array of json values to a cmake list.
macro(_qt_internal_sbom_get_attribution_key json_key out_var out_prefix)
    cmake_parse_arguments(arg "IS_MULTI_VALUE" "" "" ${ARGN})

    string(JSON "${out_var}" ERROR_VARIABLE get_error GET "${contents}" ${indices} "${json_key}")
    if(NOT "${${out_var}}" STREQUAL "" AND NOT get_error)
        set(extracted_value "${${out_var}}")

        if(arg_IS_MULTI_VALUE)
            _qt_internal_sbom_handle_attribution_json_array("${contents}"
                EXTRACTED_VALUE "${extracted_value}"
                INDICES ${indices} ${json_key}
                OUT_VAR value_list
            )
            if(value_list)
                set(extracted_value "${value_list}")
            endif()
        endif()

        _qt_internal_sbom_escape_json_content("${extracted_value}" escaped_content)

        set(${out_prefix}_${out_var} "${escaped_content}" PARENT_SCOPE)
        list(APPEND variable_names "${out_var}")

        unset(extracted_value)
        unset(escaped_content)
        unset(value_list)
    endif()
endmacro()

# Set sbom project name for the root project.
function(_qt_internal_sbom_set_root_project_name project_name)
    set_property(GLOBAL PROPERTY _qt_internal_sbom_repo_project_name "${project_name}")
endfunction()

# Get repo project_name spdx id reference, needs to start with Package- to be NTIA compliant.
function(_qt_internal_sbom_get_root_project_name_for_spdx_id out_var)
    _qt_internal_sbom_get_root_project_name_lower_case(repo_project_name_lowercase)
    set(sbom_repo_project_name "Package-${repo_project_name_lowercase}")
    set(${out_var} "${sbom_repo_project_name}" PARENT_SCOPE)
endfunction()

# Just a lower case sbom project name.
function(_qt_internal_sbom_get_root_project_name_lower_case out_var)
    get_cmake_property(project_name _qt_internal_sbom_repo_project_name)

    if(NOT project_name)
        message(FATAL_ERROR "No SBOM project name was set.")
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

# Get a sanitized spdx id for a file.
# For consistency, we prefix the id with SPDXRef-PackagedFile-. This is not a requirement.
function(_qt_internal_sbom_get_file_spdx_id target out_var)
    _qt_internal_sbom_get_sanitized_spdx_id(spdx_id "SPDXRef-PackagedFile-${target}")
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
    else()
        set(package_purpose "OTHER")
    endif()
    set(${out_purpose} "${package_purpose}" PARENT_SCOPE)
endfunction()

# Get a qt spdx license expression given the id.
function(_qt_internal_sbom_get_spdx_license_expression id out_var)
    set(license "")

    # The default for modules / plugins
    if(id STREQUAL "QT_DEFAULT" OR id STREQUAL "QT_COMMERCIAL_OR_LGPL3")
        set(license "LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only")

    # For commercial only entities
    elseif(id STREQUAL "QT_COMMERCIAL")
        set(license "LicenseRef-Qt-Commercial")

    # For GPL3 only modules
    elseif(id STREQUAL "QT_COMMERCIAL_OR_GPL3")
        set(license "LicenseRef-Qt-Commercial OR GPL-3.0-only")

    # For tools and apps
    elseif(id STREQUAL "QT_COMMERCIAL_OR_GPL3_WITH_EXCEPTION")
        set(license "LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0")

    # For things like the qtmain library
    elseif(id STREQUAL "QT_COMMERCIAL_OR_BSD3")
        set(license "LicenseRef-Qt-Commercial OR BSD-3-Clause")

    # For documentation
    elseif(id STREQUAL "QT_COMMERCIAL_OR_GFDL1_3")
        set(license "LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only")

    # For examples and the like
    elseif(id STREQUAL "BSD3")
        set(license "BSD-3-Clause")

    endif()

    if(NOT license)
        message(FATAL_ERROR "No SPDX license expression found for id: ${id}")
    endif()

    set(${out_var} "${license}" PARENT_SCOPE)
endfunction()

# Get the default qt copyright.
function(_qt_internal_sbom_get_default_qt_copyright_header out_var)
    set(${out_var}
        "Copyright (C) 2024 The Qt Company Ltd."
        PARENT_SCOPE)
endfunction()

# Get the default qt package version.
function(_qt_internal_sbom_get_default_qt_package_version out_var)
    set(${out_var} "${QT_REPO_MODULE_VERSION}" PARENT_SCOPE)
endfunction()

# Get the default qt supplier.
function(_qt_internal_sbom_get_default_supplier out_var)
    set(${out_var} "TheQtCompany" PARENT_SCOPE)
endfunction()

# Get the default qt supplier url.
function(_qt_internal_sbom_get_default_supplier_url out_var)
    set(${out_var} "https://qt.io" PARENT_SCOPE)
endfunction()

# Get the default qt download location.
# If git info is available, includes the hash.
function(_qt_internal_sbom_get_qt_repo_source_download_location out_var)
    _qt_internal_sbom_get_root_project_name_lower_case(repo_project_name_lowercase)
    set(download_location "git://code.qt.io/qt/${repo_project_name_lowercase}.git")
    if(QT_SBOM_GIT_HASH)
        string(APPEND download_location "@${QT_SBOM_GIT_HASH}")
    endif()
    set(${out_var} "${download_location}" PARENT_SCOPE)
endfunction()

# Computes a security CPE for a given set of attributes.
#
# When a part is not specified, a wildcard is added.
#
# References:
# https://spdx.github.io/spdx-spec/v2.3/external-repository-identifiers/#f22-cpe23type
# https://nvlpubs.nist.gov/nistpubs/Legacy/IR/nistir7695.pdf
# https://nvd.nist.gov/products/cpe
#
# Each attribute means:
# 1. part
# 2. vendor
# 3. product
# 4. version
# 5. update
# 6. edition
# 7. language
# 8. sw_edition
# 9. target_sw
# 10. target_hw
# 11. other
function(_qt_internal_sbom_compute_security_cpe out_cpe)
    set(opt_args "")
    set(single_args
        PART
        VENDOR
        PRODUCT
        VERSION
        UPDATE
        EDITION
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    set(cpe_template "cpe:2.3:PART:VENDOR:PRODUCT:VERSION:UPDATE:EDITION:*:*:*:*:*")

    set(cpe "${cpe_template}")
    foreach(attribute_name IN LISTS single_args)
        if(arg_${attribute_name})
            set(${attribute_name}_value "${arg_${attribute_name}}")
        else()
            if(attribute_name STREQUAL "PART")
                set(${attribute_name}_value "a")
            else()
                set(${attribute_name}_value "*")
            endif()
        endif()
        string(REPLACE "${attribute_name}" "${${attribute_name}_value}" cpe "${cpe}")
    endforeach()

    set(${out_cpe} "${cpe}" PARENT_SCOPE)
endfunction()

# Computes the default security CPE for the Qt framework.
function(_qt_internal_sbom_get_cpe_qt out_var)
    _qt_internal_sbom_get_root_project_name_lower_case(repo_project_name_lowercase)
    _qt_internal_sbom_compute_security_cpe(repo_cpe
        VENDOR "qt"
        PRODUCT "${repo_project_name_lowercase}"
        VERSION "${QT_REPO_MODULE_VERSION}"
    )
    set(${out_var} "${repo_cpe}" PARENT_SCOPE)
endfunction()

# Computes the default security CPE for a given qt repository.
function(_qt_internal_sbom_get_cpe_qt_repo out_var)
    _qt_internal_sbom_compute_security_cpe(qt_cpe
        VENDOR "qt"
        PRODUCT "qt"
        VERSION "${QT_REPO_MODULE_VERSION}"
    )
    set(${out_var} "${qt_cpe}" PARENT_SCOPE)
endfunction()

# Computes the list of security CPEs for Qt, including both the repo-specific one and generic one.
function(_qt_internal_sbom_compute_security_cpe_for_qt out_cpe_list)
    set(cpe_list "")

    _qt_internal_sbom_get_cpe_qt(repo_cpe)
    list(APPEND cpe_list "${repo_cpe}")

    _qt_internal_sbom_get_cpe_qt_repo(qt_cpe)
    list(APPEND cpe_list "${qt_cpe}")

    set(${out_cpe_list} "${cpe_list}" PARENT_SCOPE)
endfunction()

# Parse purl arguments for a specific purl variant, e.g. for parsing all values of arg_PURL_QT_ARGS.
# arguments_var_name is the variable name that contains the args.
macro(_qt_internal_sbom_parse_purl_variant_options prefix arguments_var_name)
    _qt_internal_get_sbom_purl_parsing_options(purl_opt_args purl_single_args purl_multi_args)

    cmake_parse_arguments(arg "${purl_opt_args}" "${purl_single_args}" "${purl_multi_args}"
        ${${arguments_var_name}})
    _qt_internal_validate_all_args_are_parsed(arg)
endmacro()

# Returns a vcs url where for purls where qt entities of the current repo are hosted.
function(_qt_internal_sbom_get_qt_entity_vcs_url target)
    set(opt_args "")
    set(single_args
        REPO_NAME
        VERSION
        OUT_VAR
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_REPO_NAME)
        message(FATAL_ERROR "REPO_NAME must be set")
    endif()

    if(NOT arg_OUT_VAR)
        message(FATAL_ERROR "OUT_VAR must be set")
    endif()

    set(version_part "")
    if(arg_VERSION)
        set(version_part "@${arg_VERSION}")
    endif()

    set(vcs_url "https://code.qt.io/qt/${arg_REPO_NAME}.git${version_part}")
    set(${arg_OUT_VAR} "${vcs_url}" PARENT_SCOPE)
endfunction()

# Returns a relative path to the source where the target was created, to be embedded into a
# mirror purl as a subpath.
function(_qt_internal_sbom_get_qt_entity_repo_source_dir target)
    set(opt_args "")
    set(single_args
        OUT_VAR
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_OUT_VAR)
        message(FATAL_ERROR "OUT_VAR must be set")
    endif()

    get_target_property(repo_source_dir "${target}" SOURCE_DIR)

    # Get the path relative to the PROJECT_SOURCE_DIR
    file(RELATIVE_PATH relative_repo_source_dir "${PROJECT_SOURCE_DIR}" "${repo_source_dir}")

    set(sub_path "${relative_repo_source_dir}")
    set(${arg_OUT_VAR} "${sub_path}" PARENT_SCOPE)
endfunction()

# Handles purl arguments specified to functions like qt_internal_add_sbom.
# Currently accepts arguments for 3 variants of purls, each of which will generate a separate purl.
# If no arguments are specified, for qt entity types, default values will be chosen.
#
# Purl variants:
# - PURL_QT_ARGS
#       args to override Qt's generic purl for Qt modules or patched 3rd party libs
#       defaults to something like pkg:generic/TheQtCompany/${repo_name}-${target}@SHA1
# - PURL_MIRROR_ARGS
#       args to override Qt's mirror purl, which is hosted on github
#       defaults to something like pkg:github/qt/${repo_name}@SHA1
# - PURL_3RDPARTY_UPSTREAM_ARGS
#       args to specify a purl pointing to an upstream repo, usually to github or another forge
#       no defaults, but could look like: pkg:github/harfbuzz/harfbuzz@v8.5.0
# Example values for harfbuzz:
#     PURL_3RDPARTY_UPSTREAM_ARGS
#         PURL_TYPE "github"
#         PURL_NAMESPACE "harfbuzz"
#         PURL_NAME "harfbuzz"
#         PURL_VERSION "v8.5.0" # tag
function(_qt_internal_sbom_handle_purl_values target)
    _qt_internal_get_sbom_purl_handling_options(opt_args single_args multi_args)
    list(APPEND single_args OUT_VAR)

    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_OUT_VAR)
        message(FATAL_ERROR "OUT_VAR must be set")
    endif()

    # List of purl variants to process.
    set(purl_variants "")

    set(third_party_types
        QT_THIRD_PARTY_MODULE
        QT_THIRD_PARTY_SOURCES
    )

    if(arg_IS_QT_ENTITY_TYPE)
        # Qt entities have two purls by default, a QT generic one and a MIRROR hosted on github.
        list(APPEND purl_variants MIRROR QT)
    elseif(arg_TYPE IN_LIST third_party_types)
        # Third party libraries vendored in Qt also have at least two purls, like regular Qt
        # libraries, but might also have an upstream one.

        # The order in which the purls are generated matters for tools that consume the SBOM. Some
        # tools can only handle one PURL per package, so the first one should be the important one.
        # For now, I deem that the upstream one if present. Otherwise the github mirror.
        if(arg_PURL_3RDPARTY_UPSTREAM_ARGS)
            list(APPEND purl_variants 3RDPARTY_UPSTREAM)
        endif()

        list(APPEND purl_variants MIRROR QT)
    else()
        # If handling another entity type, handle based on whether any of the purl arguments are
        # set.
        set(known_purl_variants QT MIRROR 3RDPARTY_UPSTREAM)
        foreach(known_purl_variant IN_LIST known_purl_variants)
            if(arg_PURL_${known_purl_variant}_ARGS)
                list(APPEND purl_variants ${known_purl_variant})
            endif()
        endforeach()
    endif()

    if(arg_IS_QT_ENTITY_TYPE
            OR arg_TYPE STREQUAL "QT_THIRD_PARTY_MODULE"
            OR arg_TYPE STREQUAL "QT_THIRD_PARTY_SOURCES"
        )
        set(is_qt_purl_entity_type TRUE)
    else()
        set(is_qt_purl_entity_type FALSE)
    endif()

    _qt_internal_get_sbom_purl_parsing_options(purl_opt_args purl_single_args purl_multi_args)

    set(project_package_options "")

    foreach(purl_variant IN LISTS purl_variants)
        # Clear previous values.
        foreach(option_name IN LISTS purl_opt_args purl_single_args purl_multi_args)
            unset(arg_${option_name})
        endforeach()

        _qt_internal_sbom_parse_purl_variant_options(arg arg_PURL_${purl_variant}_ARGS)

        # Check if custom purl args were specified.
        set(purl_args_available FALSE)
        if(arg_PURL_${purl_variant}_ARGS)
            set(purl_args_available TRUE)
        endif()

        # We want to create a purl either if it's one of Qt's entities and one of it's default
        # purl types, or if custom args were specified.
        set(consider_purl_processing FALSE)
        if((purl_args_available OR is_qt_purl_entity_type) AND NOT arg_NO_PURL)
            set(consider_purl_processing TRUE)
        endif()

        if(consider_purl_processing)
            set(purl_args "")

            # Override the purl version with the package version.
            if(arg_PURL_USE_PACKAGE_VERSION AND arg_VERSION)
                set(arg_PURL_VERSION "${arg_VERSION}")
            endif()

            # Append a vcs_url to the qualifiers if specified.
            if(arg_PURL_VCS_URL)
                list(APPEND arg_PURL_QUALIFIERS "vcs_url=${arg_PURL_VCS_URL}")
            endif()

            _qt_internal_forward_function_args(
                FORWARD_APPEND
                FORWARD_PREFIX arg
                FORWARD_OUT_VAR purl_args
                FORWARD_OPTIONS
                    ${purl_opt_args}
                FORWARD_SINGLE
                    ${purl_single_args}
                FORWARD_MULTI
                    ${purl_multi_args}
            )

            # Qt entity types get special treatment purl.
            if(is_qt_purl_entity_type AND NOT arg_NO_DEFAULT_QT_PURL AND
                    (purl_variant STREQUAL "QT" OR purl_variant STREQUAL "MIRROR"))
                _qt_internal_sbom_get_root_project_name_lower_case(repo_project_name_lowercase)

                # Add a vcs_url to the generic QT variant.
                if(purl_variant STREQUAL "QT")
                    _qt_internal_sbom_get_qt_entity_vcs_url(${target}
                        REPO_NAME "${repo_project_name_lowercase}"
                        VERSION "${QT_SBOM_GIT_HASH_SHORT}" # can be empty
                        OUT_VAR vcs_url)
                    list(APPEND purl_args PURL_QUALIFIERS "vcs_url=${vcs_url}")
                endif()

                # Add the subdirectory path where the target was created as a custom qualifier.
                _qt_internal_sbom_get_qt_entity_repo_source_dir(${target} OUT_VAR sub_path)
                list(APPEND purl_args PURL_SUBPATH "${sub_path}")

                # Add the target name as a custom qualifer.
                list(APPEND purl_args PURL_QUALIFIERS "library_name=${target}")

                # Get purl args the Qt entity type, taking into account defaults.
                _qt_internal_sbom_get_qt_entity_purl_args(${target}
                    NAME "${repo_project_name_lowercase}-${target}"
                    REPO_NAME "${repo_project_name_lowercase}"
                    SUPPLIER "${arg_SUPPLIER}"
                    VERSION "${QT_SBOM_GIT_HASH_SHORT}" # can be empty
                    PURL_VARIANT "${purl_variant}"
                    ${purl_args}
                    OUT_VAR purl_args
                )
            endif()

            _qt_internal_sbom_assemble_purl(${target}
                ${purl_args}
                OUT_VAR package_manager_external_ref
            )
            list(APPEND project_package_options ${package_manager_external_ref})
        endif()
    endforeach()

    set(direct_values
        PURL_QT_VALUES
        PURL_MIRROR_VALUES
        PURL_3RDPARTY_UPSTREAM_VALUES
    )

    foreach(direct_value IN LISTS direct_values)
        if(arg_${direct_value})
            set(direct_values_per_type "")
            foreach(direct_value IN LISTS arg_${direct_value})
                _qt_internal_sbom_get_purl_value_extref(
                    VALUE "${direct_value}" OUT_VAR package_manager_external_ref)

                list(APPEND direct_values_per_type ${package_manager_external_ref})
            endforeach()
            # The order in which the purls are generated, matters for tools that consume the SBOM.
            # Some tools can only handle one PURL per package, so the first one should be the
            # important one.
            # For now, I deem that the directly specified ones (probably via a qt_attribution.json
            # file) are the more important ones. So we prepend them.
            list(PREPEND project_package_options ${direct_values_per_type})
        endif()
    endforeach()

    set(${arg_OUT_VAR} "${project_package_options}" PARENT_SCOPE)
endfunction()

# Gets a list of arguments to pass to _qt_internal_sbom_assemble_purl when handling a Qt entity
# type. The purl for Qt entity types have Qt-specific defaults, but can be overridden per purl
# component.
# The arguments are saved in OUT_VAR.
function(_qt_internal_sbom_get_qt_entity_purl_args target)
    set(opt_args "")
    set(single_args
        NAME
        REPO_NAME
        SUPPLIER
        VERSION
        PURL_VARIANT
        OUT_VAR
    )
    set(multi_args "")

    _qt_internal_get_sbom_purl_parsing_options(purl_opt_args purl_single_args purl_multi_args)
    list(APPEND opt_args ${purl_opt_args})
    list(APPEND single_args ${purl_single_args})
    list(APPEND multi_args ${purl_multi_args})

    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    set(supported_purl_variants QT MIRROR)
    if(NOT arg_PURL_VARIANT IN_LIST supported_purl_variants)
        message(FATAL_ERROR "PURL_VARIANT unknown: ${arg_PURL_VARIANT}")
    endif()

    if(arg_PURL_VARIANT STREQUAL "QT")
        set(purl_type "generic")
        set(purl_namespace "${arg_SUPPLIER}")
        set(purl_name "${arg_NAME}")
        set(purl_version "${arg_VERSION}")
    elseif(arg_PURL_VARIANT STREQUAL "MIRROR")
        set(purl_type "github")
        set(purl_namespace "qt")
        set(purl_name "${arg_REPO_NAME}")
        set(purl_version "${arg_VERSION}")
    endif()

    if(arg_PURL_TYPE)
        set(purl_type "${arg_PURL_TYPE}")
    endif()

    if(arg_PURL_NAMESPACE)
        set(purl_namespace "${arg_PURL_NAMESPACE}")
    endif()

    if(arg_PURL_NAME)
        set(purl_name "${arg_PURL_NAME}")
    endif()

    if(arg_PURL_VERSION)
        set(purl_version "${arg_PURL_VERSION}")
    endif()

    set(purl_args
        PURL_TYPE "${purl_type}"
        PURL_NAMESPACE "${purl_namespace}"
        PURL_NAME "${purl_name}"
        PURL_VERSION "${purl_version}"
    )

    if(arg_PURL_QUALIFIERS)
        list(APPEND purl_args PURL_QUALIFIERS "${arg_PURL_QUALIFIERS}")
    endif()

    if(arg_PURL_SUBPATH)
        list(APPEND purl_args PURL_SUBPATH "${arg_PURL_SUBPATH}")
    endif()

    set(${arg_OUT_VAR} "${purl_args}" PARENT_SCOPE)
endfunction()

# Assembles an external reference purl identifier.
# PURL_TYPE and PURL_NAME are required.
# Stores the result in the OUT_VAR.
# Accepted options:
#    PURL_TYPE
#    PURL_NAME
#    PURL_NAMESPACE
#    PURL_VERSION
#    PURL_SUBPATH
#    PURL_QUALIFIERS
function(_qt_internal_sbom_assemble_purl target)
    set(opt_args "")
    set(single_args
        OUT_VAR
    )
    set(multi_args "")

    _qt_internal_get_sbom_purl_parsing_options(purl_opt_args purl_single_args purl_multi_args)
    list(APPEND opt_args ${purl_opt_args})
    list(APPEND single_args ${purl_single_args})
    list(APPEND multi_args ${purl_multi_args})

    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    set(purl_scheme "pkg")

    if(NOT arg_PURL_TYPE)
        message(FATAL_ERROR "PURL_TYPE must be set")
    endif()

    if(NOT arg_PURL_NAME)
        message(FATAL_ERROR "PURL_NAME must be set")
    endif()

    if(NOT arg_OUT_VAR)
        message(FATAL_ERROR "OUT_VAR must be set")
    endif()

    # https://github.com/package-url/purl-spec
    # Spec is 'scheme:type/namespace/name@version?qualifiers#subpath'
    set(purl "${purl_scheme}:${arg_PURL_TYPE}")

    if(arg_PURL_NAMESPACE)
        string(APPEND purl "/${arg_PURL_NAMESPACE}")
    endif()

    string(APPEND purl "/${arg_PURL_NAME}")

    if(arg_PURL_VERSION)
        string(APPEND purl "@${arg_PURL_VERSION}")
    endif()

    if(arg_PURL_QUALIFIERS)
        # TODO: Note that the qualifiers are expected to be URL encoded, which this implementation
        # is not doing at the moment.
        list(JOIN arg_PURL_QUALIFIERS "&" qualifiers)
        string(APPEND purl "?${qualifiers}")
    endif()

    if(arg_PURL_SUBPATH)
        string(APPEND purl "#${arg_PURL_SUBPATH}")
    endif()

    _qt_internal_sbom_get_purl_value_extref(VALUE "${purl}" OUT_VAR result)

    set(${arg_OUT_VAR} "${result}" PARENT_SCOPE)
endfunction()

# Takes a PURL VALUE and returns an SBOM purl external reference in OUT_VAR.
function(_qt_internal_sbom_get_purl_value_extref)
    set(opt_args "")
    set(single_args
        OUT_VAR
        VALUE
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_OUT_VAR)
        message(FATAL_ERROR "OUT_VAR must be set")
    endif()

    if(NOT arg_VALUE)
        message(FATAL_ERROR "VALUE must be set")
    endif()

    # SPDX SBOM External reference type.
    set(ext_ref_prefix "PACKAGE-MANAGER purl")
    set(external_ref "${ext_ref_prefix} ${arg_VALUE}")
    set(result "EXTREF" "${external_ref}")
    set(${arg_OUT_VAR} "${result}" PARENT_SCOPE)
endfunction()

# Collects app bundle related information and paths from an executable's target properties.
# Output variables:
#    <out_var>_name bundle base name, e.g. 'Linguist'.
#    <out_var>_dir_name bundle dir name, e.g. 'Linguist.app'.
#    <out_var>_contents_dir bundle contents dir, e.g. 'Linguist.app/Contents'
#    <out_var>_contents_binary_dir bundle contents dir, e.g. 'Linguist.app/Contents/MacOS'
function(_qt_internal_get_executable_bundle_info out_var target)
    get_target_property(target_type ${target} TYPE)
    if(NOT "${target_type}" STREQUAL "EXECUTABLE")
        message(FATAL_ERROR "The target ${target} is not an executable")
    endif()

    get_target_property(output_name ${target} OUTPUT_NAME)
    if(NOT output_name)
        set(output_name "${target}")
    endif()

    set(${out_var}_name "${output_name}")
    set(${out_var}_dir_name "${${out_var}_name}.app")
    set(${out_var}_contents_dir "${${out_var}_dir_name}/Contents")
    set(${out_var}_contents_binary_dir "${${out_var}_contents_dir}/MacOS")

    set(${out_var}_name "${${out_var}_name}" PARENT_SCOPE)
    set(${out_var}_dir_name "${${out_var}_dir_name}" PARENT_SCOPE)
    set(${out_var}_contents_dir "${${out_var}_contents_dir}" PARENT_SCOPE)
    set(${out_var}_contents_binary_dir "${${out_var}_contents_binary_dir}" PARENT_SCOPE)
endfunction()

# Helper function to add binary file to the sbom, while handling multi-config and different
# kind of paths.
# In multi-config builds, we assume that the non-default config file will be optional, because it
# might not be installed (the case for debug tools and apps in debug-and-release builds).
function(_qt_internal_sbom_handle_multi_config_target_binary_file target)
    set(opt_args "")
    set(single_args
        PATH_KIND
        PATH_SUFFIX
    )
    set(multi_args
        OPTIONS
    )

    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)
    if(is_multi_config)
        set(configs ${CMAKE_CONFIGURATION_TYPES})
    else()
        set(configs "${CMAKE_BUILD_TYPE}")
    endif()

    foreach(config IN LISTS configs)
        _qt_internal_sbom_get_and_check_multi_config_aware_single_arg_option(
            arg "${arg_PATH_KIND}" "${config}" resolved_path)
        _qt_internal_sbom_get_target_file_is_optional_in_multi_config("${config}" is_optional)
        _qt_internal_path_join(file_path "${resolved_path}" "${arg_PATH_SUFFIX}")
        _qt_internal_sbom_add_binary_file(
            "${target}"
            "${file_path}"
            ${arg_OPTIONS}
            ${is_optional}
            CONFIG ${config}
        )
    endforeach()
endfunction()

# Helper to retrieve a list of multi-config aware option names that can be parsed by the binary
# file handling function.
# For example in single config we need to parse RUNTIME_PATH, in multi-config we need to parse
# RUNTIME_PATH_DEBUG and RUNTIME_PATH_RELEASE.
#
# Result is cached in a global property.
function(_qt_internal_sbom_get_multi_config_single_args out_var)
    get_cmake_property(single_args
        _qt_internal_sbom_multi_config_single_args)

    if(single_args)
        set(${out_var} ${single_args} PARENT_SCOPE)
        return()
    endif()

    set(single_args "")

    set(single_args_to_process
        INSTALL_PATH
        RUNTIME_PATH
        LIBRARY_PATH
        ARCHIVE_PATH
        FRAMEWORK_PATH
    )

    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)
    if(is_multi_config)
        set(configs ${CMAKE_CONFIGURATION_TYPES})
        foreach(config IN LISTS configs)
            string(TOUPPER ${config} config_upper)
            foreach(single_arg IN LISTS single_args_to_process)
                list(APPEND single_args "${single_arg}_${config_upper}")
            endforeach()
        endforeach()
    else()
        list(APPEND single_args "${single_args_to_process}")
    endif()

    set_property(GLOBAL PROPERTY
        _qt_internal_sbom_multi_config_single_args "${single_args}")
    set(${out_var} ${single_args} PARENT_SCOPE)
endfunction()

# Helper to apped a an option and a value to a list of options, while being multi-config aware.
# It appends e.g. either RUNTIME_PATH foo or RUNTIME_PATH_DEBUG foo to the out_var_args variable.
function(_qt_internal_sbom_append_multi_config_aware_single_arg_option
        arg_name arg_value config out_var_args)
    set(values "${${out_var_args}}")

    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)
    if(is_multi_config)
        string(TOUPPER ${config} config_upper)
        list(APPEND values "${arg_name}_${config_upper}" "${arg_value}")
    else()
        list(APPEND values "${arg_name}" "${arg_value}")
    endif()

    set(${out_var_args} "${values}" PARENT_SCOPE)
endfunction()

# Helper to check whether a given option was set in the outer scope, while being multi-config
# aware.
# It checks e.g. if either arg_RUNTIME_PATH or arg_RUNTIME_PATH_DEBUG is set in the outer scope.
function(_qt_internal_sbom_get_and_check_multi_config_aware_single_arg_option
        arg_prefix arg_name config out_var)
    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)

    if(is_multi_config)
        string(TOUPPER ${config} config_upper)
        set(outer_scope_var_name "${arg_prefix}_${arg_name}_${config_upper}")
        set(option_name "${arg_name}_${config_upper}")
    else()
        set(outer_scope_var_name "${arg_prefix}_${arg_name}")
        set(option_name "${arg_name}")
    endif()

    if(NOT DEFINED ${outer_scope_var_name})
        message(FATAL_ERROR "Missing ${option_name}")
    endif()

    set(${out_var} "${${outer_scope_var_name}}" PARENT_SCOPE)
endfunction()

# Checks if given config is not the first config in a multi-config build, and thus file installation
# for that config should be optional.
function(_qt_internal_sbom_is_config_optional_in_multi_config config out_var)
    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)

    if(QT_MULTI_CONFIG_FIRST_CONFIG)
        set(first_config_type "${QT_MULTI_CONFIG_FIRST_CONFIG}")
    elseif(CMAKE_CONFIGURATION_TYPES)
        list(GET CMAKE_CONFIGURATION_TYPES 0 first_config_type)
    endif()

    if(is_multi_config AND NOT (cmake_config STREQUAL first_config_type))
        set(is_optional TRUE)
    else()
        set(is_optional FALSE)
    endif()

    set(${out_var} "${is_optional}" PARENT_SCOPE)
endfunction()

# Checks if given config is not the first config in a multi-config build, and thus file installation
# for that config should be optional, sets the actual option name.
function(_qt_internal_sbom_get_target_file_is_optional_in_multi_config config out_var)
    _qt_internal_sbom_is_config_optional_in_multi_config("${config}" is_optional)

    if(is_optional)
        set(option "OPTIONAL")
    else()
        set(option "")
    endif()

    set(${out_var} "${option}" PARENT_SCOPE)
endfunction()

# Joins two license IDs with the given ${op}, avoiding parenthesis when possible.
function(_qt_internal_sbom_join_two_license_ids_with_op left_id op right_id out_var)
    if(NOT left_id)
        set(${out_var} "${right_id}" PARENT_SCOPE)
        return()
    endif()

    if(NOT right_id)
        set(${out_var} "${left_id}" PARENT_SCOPE)
        return()
    endif()

    set(value "(${left_id}) ${op} (${right_id})")
    set(${out_var} "${value}" PARENT_SCOPE)
endfunction()
