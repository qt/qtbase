# Copyright (C) 2024 The Qt Company Ltd.
# Copyright (C) 2023-2024 Jochem Rutgers
# SPDX-License-Identifier: MIT AND BSD-3-Clause

# Helper to set a single arg option to a default value if not set.
function(qt_internal_sbom_set_default_option_value option_name default)
    if(NOT arg_${option_name})
        set(arg_${option_name} "${default}" PARENT_SCOPE)
    endif()
endfunction()

# Helper to set a single arg option to a default value if not set.
# Errors out if the end value is empty. Including if the default value was empty.
function(qt_internal_sbom_set_default_option_value_and_error_if_empty option_name default)
    qt_internal_sbom_set_default_option_value("${option_name}" "${default}")
    if(NOT arg_${option_name})
        message(FATAL_ERROR "Specifying a non-empty ${option_name} is required")
    endif()
endfunction()

# Helper that returns the directory where the intermediate sbom files will be generated.
function(_qt_internal_get_current_project_sbom_dir out_var)
    set(sbom_dir "${PROJECT_BINARY_DIR}/qt_sbom")
    set(${out_var} "${sbom_dir}" PARENT_SCOPE)
endfunction()

# Helper to return the path to staging spdx file, where content will be incrementally appended to.
function(_qt_internal_get_staging_area_spdx_file_path out_var)
    _qt_internal_get_current_project_sbom_dir(sbom_dir)
    _qt_internal_sbom_get_root_project_name_lower_case(repo_project_name_lowercase)
    set(staging_area_spdx_file "${sbom_dir}/staging-${repo_project_name_lowercase}.spdx.in")
    set(${out_var} "${staging_area_spdx_file}" PARENT_SCOPE)
endfunction()

# Starts recording information for the generation of an sbom for a project.
# The intermediate files that generate the sbom are generated at cmake generation time, but are only
# actually run at build time or install time.
# The files are tracked in cmake global properties.
function(_qt_internal_sbom_begin_project_generate)
    set(opt_args "")
    set(single_args
        OUTPUT
        OUTPUT_RELATIVE_PATH
        LICENSE
        COPYRIGHT
        DOWNLOAD_LOCATION
        PROJECT
        PROJECT_FOR_SPDX_ID
        SUPPLIER
        SUPPLIER_URL
        NAMESPACE
        CPE
        OUT_VAR_PROJECT_SPDX_ID
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    string(TIMESTAMP current_utc UTC)
    string(TIMESTAMP current_year "%Y" UTC)

    qt_internal_sbom_set_default_option_value(PROJECT "${PROJECT_NAME}")

    set(default_sbom_file_name
        "${arg_PROJECT}/${arg_PROJECT}-sbom-${QT_SBOM_GIT_VERSION_PATH}.spdx")
    set(default_install_sbom_path
        "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/${default_sbom_file_name}")

    qt_internal_sbom_set_default_option_value(OUTPUT "${default_install_sbom_path}")
    qt_internal_sbom_set_default_option_value(OUTPUT_RELATIVE_PATH
        "${default_sbom_file_name}")

    qt_internal_sbom_set_default_option_value(LICENSE "NOASSERTION")
    qt_internal_sbom_set_default_option_value(PROJECT_FOR_SPDX "${PROJECT_NAME}")
    qt_internal_sbom_set_default_option_value_and_error_if_empty(SUPPLIER "")
    qt_internal_sbom_set_default_option_value(COPYRIGHT "${current_year} ${arg_SUPPLIER}")
    qt_internal_sbom_set_default_option_value_and_error_if_empty(SUPPLIER_URL
        "${PROJECT_HOMEPAGE_URL}")
    qt_internal_sbom_set_default_option_value(NAMESPACE
        "${arg_SUPPLIER}/spdxdocs/${arg_PROJECT}-${QT_SBOM_GIT_VERSION}")

    if(arg_CPE)
        set(QT_SBOM_CPE "${arg_CPE}")
    else()
        set(QT_SBOM_CPE "")
    endif()

    string(REGEX REPLACE "[^A-Za-z0-9.]+" "-" arg_PROJECT_FOR_SPDX_ID "${arg_PROJECT_FOR_SPDX_ID}")
    string(REGEX REPLACE "-+$" "" arg_PROJECT_FOR_SPDX_ID "${arg_PROJECT_FOR_SPDX_ID}")
    # Prevent collision with other generated SPDXID with -[0-9]+ suffix.
    string(REGEX REPLACE "-([0-9]+)$" "\\1" arg_PROJECT_FOR_SPDX_ID "${arg_PROJECT_FOR_SPDX_ID}")

    set(project_spdx_id "SPDXRef-${arg_PROJECT_FOR_SPDX_ID}")
    if(arg_OUT_VAR_PROJECT_SPDX_ID)
        set(${arg_OUT_VAR_PROJECT_SPDX_ID} "${project_spdx_id}" PARENT_SCOPE)
    endif()

    get_filename_component(doc_name "${arg_OUTPUT}" NAME_WLE)

    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)
    if(is_multi_config)
        set(cmake_configs "${CMAKE_CONFIGURATION_TYPES}")
    else()
        set(cmake_configs "${CMAKE_BUILD_TYPE}")
    endif()

    qt_internal_sbom_set_default_option_value(DOWNLOAD_LOCATION "NOASSERTION")

    set(content
        "SPDXVersion: SPDX-2.3
DataLicense: CC0-1.0
SPDXID: SPDXRef-DOCUMENT
DocumentName: ${doc_name}
DocumentNamespace: ${arg_NAMESPACE}
Creator: Organization: ${arg_SUPPLIER}
Creator: Tool: Qt Build System
CreatorComment: <text>This SPDX document was created from CMake ${CMAKE_VERSION}, using the qt
build system from https://code.qt.io/cgit/qt/qtbase.git/tree/cmake/QtPublicSbomHelpers.cmake</text>
Created: ${current_utc}\${QT_SBOM_EXTERNAL_DOC_REFS}

PackageName: ${CMAKE_CXX_COMPILER_ID}
SPDXID: SPDXRef-compiler
PackageVersion: ${CMAKE_CXX_COMPILER_VERSION}
PackageDownloadLocation: NOASSERTION
PackageLicenseConcluded: NOASSERTION
PackageLicenseDeclared: NOASSERTION
PackageCopyrightText: NOASSERTION
PackageSupplier: Organization: Anonymous
FilesAnalyzed: false
PackageSummary: <text>The compiler as identified by CMake, running on ${CMAKE_HOST_SYSTEM_NAME} (${CMAKE_HOST_SYSTEM_PROCESSOR})</text>
PrimaryPackagePurpose: APPLICATION
Relationship: SPDXRef-compiler BUILD_DEPENDENCY_OF ${project_spdx_id}
RelationshipComment: <text>${project_spdx_id} is built by compiler ${CMAKE_CXX_COMPILER_ID} (${CMAKE_CXX_COMPILER}) version ${CMAKE_CXX_COMPILER_VERSION}</text>

PackageName: ${arg_PROJECT}
SPDXID: ${project_spdx_id}
ExternalRef: SECURITY cpe23Type ${QT_SBOM_CPE}
ExternalRef: PACKAGE-MANAGER purl pkg:generic/${arg_SUPPLIER}/${arg_PROJECT}@${QT_SBOM_GIT_VERSION}
PackageVersion: ${QT_SBOM_GIT_VERSION}
PackageSupplier: Organization: ${arg_SUPPLIER}
PackageDownloadLocation: ${arg_DOWNLOAD_LOCATION}
PackageLicenseConcluded: ${arg_LICENSE}
PackageLicenseDeclared: ${arg_LICENSE}
PackageCopyrightText: ${arg_COPYRIGHT}
PackageHomePage: ${arg_SUPPLIER_URL}
PackageComment: <text>Built by CMake ${CMAKE_VERSION} with ${cmake_configs} configuration for ${CMAKE_SYSTEM_NAME} (${CMAKE_SYSTEM_PROCESSOR})</text>
PackageVerificationCode: \${QT_SBOM_VERIFICATION_CODE}
BuiltDate: ${current_utc}
Relationship: SPDXRef-DOCUMENT DESCRIBES ${project_spdx_id}
")

    # Create the directory that will contain all sbom related files.
    _qt_internal_get_current_project_sbom_dir(sbom_dir)
    file(MAKE_DIRECTORY "${sbom_dir}")

    # Generate project document intro spdx file.
    _qt_internal_sbom_get_root_project_name_lower_case(repo_project_name_lowercase)
    set(document_intro_file_name
        "${sbom_dir}/SPDXRef-DOCUMENT-${repo_project_name_lowercase}.spdx.in")
    file(GENERATE OUTPUT "${document_intro_file_name}" CONTENT "${content}")

    # This is the file that will be incrementally assembled by having content appended to it.
    _qt_internal_get_staging_area_spdx_file_path(staging_area_spdx_file)

    get_filename_component(output_file_name_without_ext "${arg_OUTPUT}" NAME_WLE)
    get_filename_component(output_file_ext "${arg_OUTPUT}" LAST_EXT)

    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)
    if(is_multi_config)
        set(multi_config_suffix "-$<CONFIG>")
    else()
        set(multi_config_suffix "")
    endif()

    set(computed_sbom_file_name_without_ext "${output_file_name_without_ext}${multi_config_suffix}")
    set(computed_sbom_file_name "${output_file_name_without_ext}${output_file_ext}")

    # In a super build, put all the build time sboms into the same dir in qtbase.
    if(QT_SUPERBUILD)
        set(build_sbom_dir "${QtBase_BINARY_DIR}/qt_sbom")
    else()
        set(build_sbom_dir "${sbom_dir}")
    endif()

    get_filename_component(output_relative_dir "${arg_OUTPUT_RELATIVE_PATH}" DIRECTORY)

    set(build_sbom_path
        "${build_sbom_dir}/${output_relative_dir}/${computed_sbom_file_name}")
    set(build_sbom_path_without_ext
        "${build_sbom_dir}/${output_relative_dir}/${computed_sbom_file_name_without_ext}")

    set(install_sbom_path "${arg_OUTPUT}")

    get_filename_component(install_sbom_dir "${install_sbom_path}" DIRECTORY)
    set(install_sbom_path_without_ext "${install_sbom_dir}/${output_file_name_without_ext}")

    # Create cmake file to append the document intro spdx to the staging file.
    set(create_staging_file "${sbom_dir}/append_document_to_staging${multi_config_suffix}.cmake")
    set(content "
        cmake_minimum_required(VERSION 3.16)
        message(STATUS \"Starting SBOM generation in build dir: ${staging_area_spdx_file}\")
        set(QT_SBOM_EXTERNAL_DOC_REFS \"\")
        file(READ \"${document_intro_file_name}\" content)
        # Override any previous file because we're starting from scratch.
        file(WRITE \"${staging_area_spdx_file}\" \"\${content}\")
")
    file(GENERATE OUTPUT "${create_staging_file}" CONTENT "${content}")

    set_property(GLOBAL PROPERTY _qt_sbom_project_name "${arg_PROJECT}")

    set_property(GLOBAL PROPERTY _qt_sbom_build_output_path "${build_sbom_path}")
    set_property(GLOBAL PROPERTY _qt_sbom_build_output_path_without_ext
        "${build_sbom_path_without_ext}")

    set_property(GLOBAL PROPERTY _qt_sbom_install_output_path "${install_sbom_path}")
    set_property(GLOBAL PROPERTY _qt_sbom_install_output_path_without_ext
        "${install_sbom_path_without_ext}")

    set_property(GLOBAL APPEND PROPERTY _qt_sbom_cmake_include_files "${create_staging_file}")

    set_property(GLOBAL PROPERTY _qt_sbom_spdx_id_count 0)
endfunction()

# Signals the end of recording sbom information for a project.
# Creates an 'sbom' custom target to generate an incomplete sbom at build time (no checksums).
# Creates install rules to install a complete (with checksums) sbom.
# Also allows running various post-installation steps like NTIA validation, auditing, json
# generation, etc
function(_qt_internal_sbom_end_project_generate)
    set(opt_args
        GENERATE_JSON
        GENERATE_SOURCE_SBOM
        VERIFY
        LINT_SOURCE_SBOM
        LINT_SOURCE_SBOM_NO_ERROR
        SHOW_TABLE
        AUDIT
        AUDIT_NO_ERROR
    )
    set(single_args "")
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    get_property(sbom_build_output_path GLOBAL PROPERTY _qt_sbom_build_output_path)
    get_property(sbom_build_output_path_without_ext GLOBAL PROPERTY
        _qt_sbom_build_output_path_without_ext)

    get_property(sbom_install_output_path GLOBAL PROPERTY _qt_sbom_install_output_path)
    get_property(sbom_install_output_path_without_ext GLOBAL PROPERTY
        _qt_sbom_install_output_path_without_ext)

    if(NOT sbom_build_output_path)
        message(FATAL_ERROR "Call _qt_internal_sbom_begin_project() first")
    endif()

    _qt_internal_get_staging_area_spdx_file_path(staging_area_spdx_file)

    if((arg_GENERATE_JSON OR arg_VERIFY) AND NOT QT_INTERNAL_NO_SBOM_PYTHON_OPS)
        _qt_internal_sbom_find_python()
        _qt_internal_sbom_find_python_dependencies()
    endif()

    if(arg_GENERATE_JSON AND NOT QT_INTERNAL_NO_SBOM_PYTHON_OPS)
        _qt_internal_sbom_generate_json()
    endif()

    if(arg_VERIFY AND NOT QT_INTERNAL_NO_SBOM_PYTHON_OPS)
        _qt_internal_sbom_verify_valid_and_ntia_compliant()
    endif()

    if(arg_SHOW_TABLE AND NOT QT_INTERNAL_NO_SBOM_PYTHON_OPS)
        _qt_internal_sbom_find_python_dependency_program(NAME sbom2doc REQUIRED)
        _qt_internal_sbom_show_table()
    endif()

    if(arg_AUDIT AND NOT QT_INTERNAL_NO_SBOM_PYTHON_OPS)
        set(audit_no_error_option "")
        if(arg_AUDIT_NO_ERROR)
            set(audit_no_error_option NO_ERROR)
        endif()
        _qt_internal_sbom_find_python_dependency_program(NAME sbomaudit REQUIRED)
        _qt_internal_sbom_audit(${audit_no_error_option})
    endif()

    if(arg_GENERATE_SOURCE_SBOM AND NOT QT_INTERNAL_NO_SBOM_PYTHON_OPS)
        _qt_internal_sbom_find_python_dependency_program(NAME reuse REQUIRED)
        _qt_internal_sbom_generate_reuse_source_sbom()
    endif()

    if(arg_LINT_SOURCE_SBOM AND NOT QT_INTERNAL_NO_SBOM_PYTHON_OPS)
        set(lint_no_error_option "")
        if(arg_LINT_SOURCE_SBOM_NO_ERROR)
            set(lint_no_error_option NO_ERROR)
        endif()
        _qt_internal_sbom_find_python_dependency_program(NAME reuse REQUIRED)
        _qt_internal_sbom_run_reuse_lint(
            ${lint_no_error_option}
            BUILD_TIME_SCRIPT_PATH_OUT_VAR reuse_lint_script
        )
    endif()

    get_cmake_property(cmake_include_files _qt_sbom_cmake_include_files)
    get_cmake_property(cmake_end_include_files _qt_sbom_cmake_end_include_files)
    get_cmake_property(cmake_post_generation_include_files
        _qt_sbom_cmake_post_generation_include_files)
    get_cmake_property(cmake_verify_include_files _qt_sbom_cmake_verify_include_files)

    set(includes "")
    if(cmake_include_files)
        foreach(cmake_include_file IN LISTS cmake_include_files)
            list(APPEND includes "include(\"${cmake_include_file}\")")
        endforeach()
    endif()

    if(cmake_end_include_files)
        foreach(cmake_include_file IN LISTS cmake_end_include_files)
            list(APPEND includes "include(\"${cmake_include_file}\")")
        endforeach()
    endif()

    list(JOIN includes "\n" includes)

    # Post generation includes are included for both build and install time sboms, after
    # sbom generation has finished.
    set(post_generation_includes "")
    if(cmake_post_generation_include_files)
        foreach(cmake_include_file IN LISTS cmake_post_generation_include_files)
            list(APPEND post_generation_includes "include(\"${cmake_include_file}\")")
        endforeach()
    endif()

    list(JOIN post_generation_includes "\n" post_generation_includes)

    # Verification only makes sense on installation, where the checksums are present.
    set(verify_includes "")
    if(cmake_verify_include_files)
        foreach(cmake_include_file IN LISTS cmake_verify_include_files)
            list(APPEND verify_includes "include(\"${cmake_include_file}\")")
        endforeach()
    endif()
    list(JOIN verify_includes "\n" verify_includes)

    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)
    if(is_multi_config)
        set(multi_config_suffix "-$<CONFIG>")
    else()
        set(multi_config_suffix "")
    endif()

    _qt_internal_get_current_project_sbom_dir(sbom_dir)
    set(content "
        # QT_SBOM_BUILD_TIME be set to FALSE at install time, so don't override if it's set.
        # This allows reusing the same cmake file for both build and install.
        if(NOT DEFINED QT_SBOM_BUILD_TIME)
            set(QT_SBOM_BUILD_TIME TRUE)
        endif()
        if(NOT QT_SBOM_OUTPUT_PATH)
            set(QT_SBOM_OUTPUT_PATH \"${sbom_build_output_path}\")
            set(QT_SBOM_OUTPUT_PATH_WITHOUT_EXT \"${sbom_build_output_path_without_ext}\")
        endif()
        set(QT_SBOM_VERIFICATION_CODES \"\")
        ${includes}
        if(QT_SBOM_BUILD_TIME)
            message(STATUS \"Finalizing SBOM generation in build dir: \${QT_SBOM_OUTPUT_PATH}\")
            configure_file(\"${staging_area_spdx_file}\" \"\${QT_SBOM_OUTPUT_PATH}\")
            ${post_generation_includes}
        endif()
")
    set(assemble_sbom "${sbom_dir}/assemble_sbom${multi_config_suffix}.cmake")
    file(GENERATE OUTPUT "${assemble_sbom}" CONTENT "${content}")

    if(NOT TARGET sbom)
        add_custom_target(sbom)
    endif()

    _qt_internal_sbom_get_root_project_name_lower_case(repo_project_name_lowercase)

    # Create a build target to create a build-time sbom (no verification codes or sha1s).
    set(repo_sbom_target "sbom_${repo_project_name_lowercase}")
    set(comment "")
    string(APPEND comment "Assembling build time SPDX document without checksums for "
        "${repo_project_name_lowercase}. Just for testing.")
    add_custom_target(${repo_sbom_target}
        COMMAND "${CMAKE_COMMAND}" -P "${assemble_sbom}"
        COMMENT "${comment}"
        VERBATIM
        USES_TERMINAL # To avoid running two configs of the command in parallel
    )

    get_cmake_property(qt_repo_deps _qt_repo_deps_${repo_project_name_lowercase})
    if(qt_repo_deps)
        foreach(repo_dep IN LISTS qt_repo_deps)
            set(repo_dep_sbom "sbom_${repo_dep}")
            if(TARGET "${repo_dep_sbom}")
                add_dependencies(${repo_sbom_target} ${repo_dep_sbom})
            endif()
        endforeach()
    endif()

    add_dependencies(sbom ${repo_sbom_target})

    # Add 'reuse lint' per-repo custom targets.
    if(arg_LINT_SOURCE_SBOM AND NOT QT_INTERNAL_NO_SBOM_PYTHON_OPS)
        if(NOT TARGET reuse_lint)
            add_custom_target(reuse_lint)
        endif()

        set(comment "Running 'reuse lint' for '${repo_project_name_lowercase}'.")
        add_custom_target(${repo_sbom_target}_reuse_lint
            COMMAND "${CMAKE_COMMAND}" -P "${reuse_lint_script}"
            COMMENT "${comment}"
            VERBATIM
            USES_TERMINAL # To avoid running multiple lints in parallel
        )
        add_dependencies(reuse_lint ${repo_sbom_target}_reuse_lint)
    endif()

    set(extra_code_begin "")
    set(extra_code_inner_end "")

    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)
    if(is_multi_config)
        set(configs ${CMAKE_CONFIGURATION_TYPES})

        set(install_markers_dir "${sbom_dir}")
        set(install_marker_path "${install_markers_dir}/finished_install-$<CONFIG>.cmake")

        set(install_marker_code "
            message(STATUS \"Writing install marker for config $<CONFIG>: ${install_marker_path} \")
            file(WRITE \"${install_marker_path}\" \"\")
")

        install(CODE "${install_marker_code}" COMPONENT sbom)
        if(QT_SUPERBUILD)
            install(CODE "${install_marker_code}" COMPONENT "sbom_${repo_project_name_lowercase}"
                EXCLUDE_FROM_ALL)
        endif()

        set(install_markers "")
        foreach(config IN LISTS configs)
            set(marker_path "${install_markers_dir}/finished_install-${config}.cmake")
            list(APPEND install_markers "${marker_path}")
            # Remove the markers on reconfiguration, just in case there are stale ones.
            if(EXISTS "${marker_path}")
                file(REMOVE "${marker_path}")
            endif()
        endforeach()

        set(extra_code_begin "
        set(QT_SBOM_INSTALL_MARKERS \"${install_markers}\")
        foreach(QT_SBOM_INSTALL_MARKER IN LISTS QT_SBOM_INSTALL_MARKERS)
            if(NOT EXISTS \"\${QT_SBOM_INSTALL_MARKER}\")
                set(QT_SBOM_INSTALLED_ALL_CONFIGS FALSE)
            endif()
        endforeach()
")
        set(extra_code_inner_end "
            foreach(QT_SBOM_INSTALL_MARKER IN LISTS QT_SBOM_INSTALL_MARKERS)
                message(STATUS
                    \"Removing install marker: \${QT_SBOM_INSTALL_MARKER} \")
                file(REMOVE \"\${QT_SBOM_INSTALL_MARKER}\")
            endforeach()
")
    endif()

    # Allow skipping checksum computation for testing purposes, while installing just the sbom
    # documents, without requiring to build and install all the actual files.
    if(QT_INTERNAL_SBOM_FAKE_CHECKSUM)
        string(APPEND extra_code_begin "
            set(QT_SBOM_FAKE_CHECKSUM TRUE)")
    endif()

    set(assemble_sbom_install "
        set(QT_SBOM_INSTALLED_ALL_CONFIGS TRUE)
        ${extra_code_begin}
        if(QT_SBOM_INSTALLED_ALL_CONFIGS)
            set(QT_SBOM_BUILD_TIME FALSE)
            set(QT_SBOM_OUTPUT_PATH \"${sbom_install_output_path}\")
            set(QT_SBOM_OUTPUT_PATH_WITHOUT_EXT \"${sbom_install_output_path_without_ext}\")
            include(\"${assemble_sbom}\")
            list(SORT QT_SBOM_VERIFICATION_CODES)
            string(REPLACE \";\" \"\" QT_SBOM_VERIFICATION_CODES \"\${QT_SBOM_VERIFICATION_CODES}\")
            file(WRITE \"${sbom_dir}/verification.txt\" \"\${QT_SBOM_VERIFICATION_CODES}\")
            file(SHA1 \"${sbom_dir}/verification.txt\" QT_SBOM_VERIFICATION_CODE)
            message(STATUS \"Finalizing SBOM generation in install dir: \${QT_SBOM_OUTPUT_PATH}\")
            configure_file(\"${staging_area_spdx_file}\" \"\${QT_SBOM_OUTPUT_PATH}\")
            ${post_generation_includes}
            ${verify_includes}
            ${extra_code_inner_end}
        else()
            message(STATUS \"Skipping SBOM finalization because not all configs were installed.\")
        endif()
")

    install(CODE "${assemble_sbom_install}" COMPONENT sbom)
    if(QT_SUPERBUILD)
        install(CODE "${assemble_sbom_install}" COMPONENT "sbom_${repo_project_name_lowercase}"
            EXCLUDE_FROM_ALL)
    endif()

    # Clean up properties, so that they are empty for possible next repo in a top-level build.
    set_property(GLOBAL PROPERTY _qt_sbom_cmake_include_files "")
    set_property(GLOBAL PROPERTY _qt_sbom_cmake_end_include_files "")
    set_property(GLOBAL PROPERTY _qt_sbom_cmake_post_generation_include_files "")
    set_property(GLOBAL PROPERTY _qt_sbom_cmake_verify_include_files "")
endfunction()

# Helper to add info about a file to the sbom.
# Targets are backed by multiple files in multi-config builds. To support multi-config,
# we generate a -$<CONFIG> file for each config, but we only include / install the one that is
# specified via the CONFIG option.
# For build time sboms, we skip checking file existence and sha1 computation, because the files
# are not installed yet.
function(_qt_internal_sbom_generate_add_file)
    set(opt_args
        OPTIONAL
    )
    set(single_args
        FILENAME
        FILETYPE
        RELATIONSHIP
        SPDXID
        CONFIG
        LICENSE
        COPYRIGHT
        INSTALL_PREFIX
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    qt_internal_sbom_set_default_option_value_and_error_if_empty(FILENAME "")
    qt_internal_sbom_set_default_option_value_and_error_if_empty(FILETYPE "")

    _qt_internal_sbom_get_and_check_spdx_id(
        VARIABLE arg_SPDXID
        CHECK "${arg_SPDXID}"
        HINTS "SPDXRef-${arg_FILENAME}"
    )

    qt_internal_sbom_set_default_option_value(LICENSE "NOASSERTION")
    qt_internal_sbom_set_default_option_value(COPYRIGHT "NOASSERTION")

    get_property(sbom_project_name GLOBAL PROPERTY _qt_sbom_project_name)
    if(NOT sbom_project_name)
        message(FATAL_ERROR "Call _qt_internal_sbom_begin_project() first")
    endif()
    if(NOT arg_RELATIONSHIP)
        set(arg_RELATIONSHIP "SPDXRef-${sbom_project_name} CONTAINS ${arg_SPDXID}")
    else()
        string(REPLACE
            "@QT_SBOM_LAST_SPDXID@" "${arg_SPDXID}" arg_RELATIONSHIP "${arg_RELATIONSHIP}")
    endif()

    set(fields "")

    if(arg_LICENSE)
        set(fields "${fields}
LicenseConcluded: ${arg_LICENSE}"
        )
    else()
        set(fields "${fields}
LicenseConcluded: NOASSERTION"
        )
    endif()

    if(arg_COPYRIGHT)
        set(fields "${fields}
FileCopyrightText: ${arg_COPYRIGHT}"
        )
    else()
        set(fields "${fields}
FileCopyrightText: NOASSERTION"
        )
    endif()

    set(file_suffix_to_generate "")
    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)
    if(is_multi_config)
        set(file_suffix_to_generate "-$<CONFIG>")
    endif()

    if(arg_CONFIG)
        set(file_suffix_to_install "-${arg_CONFIG}")
    else()
        set(file_suffix_to_install "")
    endif()

    _qt_internal_get_staging_area_spdx_file_path(staging_area_spdx_file)

    if(arg_INSTALL_PREFIX)
        set(install_prefix "${arg_INSTALL_PREFIX}")
    else()
        set(install_prefix "${CMAKE_INSTALL_PREFIX}")
    endif()

    set(content "
        if(NOT EXISTS $ENV{DESTDIR}${install_prefix}/${arg_FILENAME}
                AND NOT QT_SBOM_BUILD_TIME AND NOT QT_SBOM_FAKE_CHECKSUM)
            if(NOT ${arg_OPTIONAL})
                message(FATAL_ERROR \"Cannot find ${arg_FILENAME}\")
            endif()
        else()
            if(NOT QT_SBOM_BUILD_TIME)
                if(QT_SBOM_FAKE_CHECKSUM)
                    set(sha1 \"158942a783ee1095eafacaffd93de73edeadbeef\")
                else()
                    file(SHA1 $ENV{DESTDIR}${install_prefix}/${arg_FILENAME} sha1)
                endif()
                list(APPEND QT_SBOM_VERIFICATION_CODES \${sha1})
            endif()
            file(APPEND \"${staging_area_spdx_file}\"
\"
FileName: ./${arg_FILENAME}
SPDXID: ${arg_SPDXID}
FileType: ${arg_FILETYPE}
FileChecksum: SHA1: \${sha1}${fields}
LicenseInfoInFile: NOASSERTION
Relationship: ${arg_RELATIONSHIP}
\"
                )
        endif()
")

    _qt_internal_get_current_project_sbom_dir(sbom_dir)
    set(file_sbom "${sbom_dir}/${arg_SPDXID}${file_suffix_to_generate}.cmake")
    file(GENERATE OUTPUT "${file_sbom}" CONTENT "${content}")

    set(file_sbom_to_install "${sbom_dir}/${arg_SPDXID}${file_suffix_to_install}.cmake")
    set_property(GLOBAL APPEND PROPERTY _qt_sbom_cmake_include_files "${file_sbom_to_install}")
endfunction()

# Helper to add info about an external reference to a different project spdx sbom file.
function(_qt_internal_sbom_generate_add_external_reference)
    set(opt_args
        NO_AUTO_RELATIONSHIP
    )
    set(single_args
        EXTERNAL
        FILENAME
        RENAME
        SPDXID
        RELATIONSHIP

    )
    set(multi_args
        INSTALL_PREFIXES
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    qt_internal_sbom_set_default_option_value_and_error_if_empty(EXTERNAL "")
    qt_internal_sbom_set_default_option_value_and_error_if_empty(FILENAME "")

    if(NOT arg_SPDXID)
        get_property(spdx_id_count GLOBAL PROPERTY _qt_sbom_spdx_id_count)
        set(arg_SPDXID "DocumentRef-${spdx_id_count}")
        math(EXPR spdx_id_count "${spdx_id_count} + 1")
        set_property(GLOBAL PROPERTY _qt_sbom_spdx_id_count "${spdx_id_count}")
    endif()

    if(NOT "${arg_SPDXID}" MATCHES "^DocumentRef-[-a-zA-Z0-9]+$")
        message(FATAL_ERROR "Invalid DocumentRef \"${arg_SPDXID}\"")
    endif()

    get_property(sbom_project_name GLOBAL PROPERTY _qt_sbom_project_name)
    if(NOT sbom_project_name)
        message(FATAL_ERROR "Call _qt_internal_sbom_begin_project() first")
    endif()
    if(NOT arg_RELATIONSHIP)
        if(NOT arg_NO_AUTO_RELATIONSHIP)
            set(arg_RELATIONSHIP
                "SPDXRef-${sbom_project_name} DEPENDS_ON ${arg_SPDXID}:${arg_EXTERNAL}")
        else()
            set(arg_RELATIONSHIP "")
        endif()
    else()
        string(REPLACE
            "@QT_SBOM_LAST_SPDXID@" "${arg_SPDXID}" arg_RELATIONSHIP "${arg_RELATIONSHIP}")
    endif()

    _qt_internal_get_staging_area_spdx_file_path(staging_area_spdx_file)

    set(install_prefixes "")
    if(arg_INSTALL_PREFIXES)
        list(APPEND install_prefixes ${arg_INSTALL_PREFIXES})
    endif()
    if(QT6_INSTALL_PREFIX)
        list(APPEND install_prefixes ${QT6_INSTALL_PREFIX})
    endif()
    if(QT_ADDITIONAL_PACKAGES_PREFIX_PATH)
        list(APPEND install_prefixes ${QT_ADDITIONAL_PACKAGES_PREFIX_PATH})
    endif()
    if(QT_ADDITIONAL_SBOM_DOCUMENT_PATHS)
        list(APPEND install_prefixes ${QT_ADDITIONAL_SBOM_DOCUMENT_PATHS})
    endif()
    list(REMOVE_DUPLICATES install_prefixes)

    set(relationship_content "")
    if(arg_RELATIONSHIP)
        set(relationship_content "
        file(APPEND \"${staging_area_spdx_file}\"
    \"
    Relationship: ${arg_RELATIONSHIP}\")
")
    endif()

    # Filename may not exist yet, and it could be a generator expression.
    set(content "
        set(relative_file_name \"${arg_FILENAME}\")
        set(document_dir_paths ${install_prefixes})
        foreach(document_dir_path IN LISTS document_dir_paths)
            set(document_file_path \"\${document_dir_path}/\${relative_file_name}\")
            if(EXISTS \"\${document_file_path}\")
                break()
            endif()
        endforeach()
        if(NOT EXISTS \"\${document_file_path}\")
            message(FATAL_ERROR \"Could not find external SBOM document \${relative_file_name}\"
                \" in any of the document dir paths: \${document_dir_paths} \"
            )
        endif()
        file(SHA1 \"\${document_file_path}\" ext_sha1)
        file(READ \"\${document_file_path}\" ext_content)

        if(NOT \"\${ext_content}\" MATCHES \"[\\r\\n]DocumentNamespace:\")
            message(FATAL_ERROR \"Missing DocumentNamespace in \${document_file_path}\")
        endif()

        string(REGEX REPLACE \"^.*[\\r\\n]DocumentNamespace:[ \\t]*([^#\\r\\n]*).*$\"
                \"\\\\1\" ext_ns \"\${ext_content}\")

        list(APPEND QT_SBOM_EXTERNAL_DOC_REFS \"
ExternalDocumentRef: ${arg_SPDXID} \${ext_ns} SHA1: \${ext_sha1}\")

        ${relationship_content}
")

    _qt_internal_get_current_project_sbom_dir(sbom_dir)
    set(ext_ref_sbom "${sbom_dir}/${arg_SPDXID}.cmake")
    file(GENERATE OUTPUT "${ext_ref_sbom}" CONTENT "${content}")

    set_property(GLOBAL APPEND PROPERTY _qt_sbom_cmake_end_include_files "${ext_ref_sbom}")
endfunction()

# Helper to add info about a package to the sbom. Usually a package is a mapping to a cmake target.
function(_qt_internal_sbom_generate_add_package)
    set(opt_args
        CONTAINS_FILES
    )
    set(single_args
        PACKAGE
        VERSION
        LICENSE_DECLARED
        LICENSE_CONCLUDED
        COPYRIGHT
        DOWNLOAD_LOCATION
        RELATIONSHIP
        SPDXID
        SUPPLIER
        PURPOSE
        COMMENT
    )
    set(multi_args
        EXTREF
        CPE
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    qt_internal_sbom_set_default_option_value_and_error_if_empty(PACKAGE "")

    _qt_internal_sbom_get_and_check_spdx_id(
        VARIABLE arg_SPDXID
        CHECK "${arg_SPDXID}"
        HINTS "SPDXRef-${arg_PACKAGE}"
    )

    qt_internal_sbom_set_default_option_value(DOWNLOAD_LOCATION "NOASSERTION")
    qt_internal_sbom_set_default_option_value(VERSION "unknown")
    qt_internal_sbom_set_default_option_value(SUPPLIER "Person: Anonymous")
    qt_internal_sbom_set_default_option_value(LICENSE_DECLARED "NOASSERTION")
    qt_internal_sbom_set_default_option_value(LICENSE_CONCLUDED "NOASSERTION")
    qt_internal_sbom_set_default_option_value(COPYRIGHT "NOASSERTION")
    qt_internal_sbom_set_default_option_value(PURPOSE "OTHER")

    set(fields "")

    if(arg_LICENSE_CONCLUDED)
        set(fields "${fields}
PackageLicenseConcluded: ${arg_LICENSE_CONCLUDED}"
        )
    else()
        set(fields "${fields}
PackageLicenseConcluded: NOASSERTION"
        )
    endif()

    if(arg_LICENSE_DECLARED)
        set(fields "${fields}
PackageLicenseDeclared: ${arg_LICENSE_DECLARED}"
        )
    else()
        set(fields "${fields}
PackageLicenseDeclared: NOASSERTION"
        )
    endif()

    foreach(ext_ref IN LISTS arg_EXTREF)
        set(fields "${fields}
ExternalRef: ${ext_ref}"
        )
    endforeach()

    if(arg_CONTAINS_FILES)
        set(fields "${fields}
FilesAnalyzed: true"
        )
    else()
        set(fields "${fields}
FilesAnalyzed: false"
        )
    endif()

    if(arg_COPYRIGHT)
        set(fields "${fields}
PackageCopyrightText: ${arg_COPYRIGHT}"
        )
    else()
        set(fields "${fields}
PackageCopyrightText: NOASSERTION"
        )
    endif()

    if(arg_PURPOSE)
        set(fields "${fields}
PrimaryPackagePurpose: ${arg_PURPOSE}"
        )
    else()
        set(fields "${fields}
PrimaryPackagePurpose: OTHER"
        )
    endif()

    if(arg_COMMENT)
        set(fields "${fields}
PackageComment: ${arg_COMMENT}"
        )
    endif()

    foreach(cpe IN LISTS arg_CPE)
        set(fields "${fields}
ExternalRef: SECURITY cpe23Type ${cpe}"
        )
    endforeach()

    get_property(sbom_project_name GLOBAL PROPERTY _qt_sbom_project_name)
    if(NOT sbom_project_name)
        message(FATAL_ERROR "Call _qt_internal_sbom_begin_project() first")
    endif()
    if(NOT arg_RELATIONSHIP)
        set(arg_RELATIONSHIP "SPDXRef-${sbom_project_name} CONTAINS ${arg_SPDXID}")
    else()
        string(REPLACE "@QT_SBOM_LAST_SPDXID@" "${arg_SPDXID}" arg_RELATIONSHIP "${arg_RELATIONSHIP}")
    endif()

    _qt_internal_get_staging_area_spdx_file_path(staging_area_spdx_file)

    set(content "
        file(APPEND \"${staging_area_spdx_file}\"
\"
PackageName: ${arg_PACKAGE}
SPDXID: ${arg_SPDXID}
PackageDownloadLocation: ${arg_DOWNLOAD_LOCATION}
PackageVersion: ${arg_VERSION}
PackageSupplier: ${arg_SUPPLIER}${fields}
Relationship: ${arg_RELATIONSHIP}
\"
        )
")

    _qt_internal_get_current_project_sbom_dir(sbom_dir)
    set(package_sbom "${sbom_dir}/${arg_SPDXID}.cmake")
    file(GENERATE OUTPUT "${package_sbom}" CONTENT "${content}")

    set_property(GLOBAL APPEND PROPERTY _qt_sbom_cmake_include_files "${package_sbom}")
endfunction()

# Helper to add a license text from a file or text into the sbom document.
function(_qt_internal_sbom_generate_add_license)
    set(opt_args "")
    set(single_args
        LICENSE_ID
        EXTRACTED_TEXT
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    qt_internal_sbom_set_default_option_value_and_error_if_empty(LICENSE_ID "")

    _qt_internal_sbom_get_and_check_spdx_id(
        VARIABLE arg_SPDXID
        CHECK "${arg_SPDXID}"
        HINTS "SPDXRef-${arg_LICENSE_ID}"
    )

    if(NOT arg_EXTRACTED_TEXT)
        set(licenses_dir "${PROJECT_SOURCE_DIR}/LICENSES")
        file(READ "${licenses_dir}/${arg_LICENSE_ID}.txt" arg_EXTRACTED_TEXT)
        string(PREPEND arg_EXTRACTED_TEXT "<text>")
        string(APPEND arg_EXTRACTED_TEXT "</text>")
    endif()

    _qt_internal_get_staging_area_spdx_file_path(staging_area_spdx_file)

    set(content "
        file(APPEND \"${staging_area_spdx_file}\"
\"
LicenseID: ${arg_LICENSE_ID}
ExtractedText: ${arg_EXTRACTED_TEXT}
\"
        )
")

    _qt_internal_get_current_project_sbom_dir(sbom_dir)
    set(license_sbom "${sbom_dir}/${arg_SPDXID}.cmake")
    file(GENERATE OUTPUT "${license_sbom}" CONTENT "${content}")

    set_property(GLOBAL APPEND PROPERTY _qt_sbom_cmake_end_include_files "${license_sbom}")
endfunction()

# Helper to retrieve a valid spdx id, given some hints.
# HINTS can be a list of values, one of which will be sanitized and used as the spdx id.
# CHECK is expected to be a valid spdx id.
function(_qt_internal_sbom_get_and_check_spdx_id)
    set(opt_args "")
    set(single_args
        VARIABLE
        CHECK
    )
    set(multi_args
        HINTS
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    qt_internal_sbom_set_default_option_value_and_error_if_empty(VARIABLE "")

    if(NOT arg_CHECK)
        get_property(spdx_id_count GLOBAL PROPERTY _qt_sbom_spdx_id_count)
        set(suffix "-${spdx_id_count}")
        math(EXPR spdx_id_count "${spdx_id_count} + 1")
        set_property(GLOBAL PROPERTY _qt_sbom_spdx_id_count "${spdx_id_count}")

        foreach(hint IN LISTS arg_HINTS)
            _qt_internal_sbom_get_sanitized_spdx_id(id "${hint}")
            if(id)
                set(id "${id}${suffix}")
                break()
            endif()
        endforeach()

        if(NOT id)
            set(id "SPDXRef${suffix}")
        endif()
    else()
        set(id "${arg_CHECK}")
    endif()

    if("${id}" MATCHES "^SPDXRef-[-]+$"
        OR (NOT "${id}" MATCHES "^SPDXRef-[-a-zA-Z0-9]+$"))
        message(FATAL_ERROR "Invalid SPDXID \"${id}\"")
    endif()

    set(${arg_VARIABLE} "${id}" PARENT_SCOPE)
endfunction()

# Helper to find the python interpreter, to be able to run post-installation steps like NTIA
# verification.
#
# Caches the found python executable in a separate cache var QT_INTERNAL_SBOM_PYTHON_EXECUTABLE, to
# avoid conflicts with any other found python package.
#
# This is intentionally a function, and not a macro, to prevent overriding the Python3_EXECUTABLE
# non-cache variable in a global scope in case if a different python is found and used for a
# different purpose (e.g. qtwebengine or qtinterfaceframework).
# The reason to use a different python is that an already found python might not be the version we
# need.
# https://gitlab.kitware.com/cmake/cmake/-/issues/21797#note_901621 claims that finding multiple
# python versions in separate directory scopes is possible, and I claim a function scope is as
# good as a directory scope.
function(_qt_internal_sbom_find_python)
    # Return early if we found a suitable python.
    if(QT_INTERNAL_SBOM_PYTHON_EXECUTABLE)
        return()
    endif()

    # Allow disabling looking for a python interpreter shipped as part of a macOS system framework.
    if(QT_INTERNAL_NO_SBOM_FIND_PYTHON_FRAMEWORK)
        set(Python_FIND_FRAMEWORK NEVER)
        set(Python3_FIND_FRAMEWORK NEVER)
    endif()

    # NTIA-compliance checker requires Python 3.9 or later.
    set(required_version "3.9")

    # Python3_VERSION would have been set by a previous find_package(Python3) call.
    set(already_found_python_version "${Python3_VERSION}")

    if(NOT already_found_python_version
            OR "${already_found_python_version}" VERSION_LESS "${required_version}")
        # Locally reset any executable that was possibly already found and is a lower version.
        # We do this to ensure we re-do the lookup, rather than error out saying a low version was
        # found.
        set(Python3_EXECUTABLE "")

        if(QT_SBOM_PYTHON_INTERP)
            set(Python3_ROOT_DIR ${QT_SBOM_PYTHON_INTERP})
        endif()

        find_package(Python3 ${required_version} REQUIRED COMPONENTS Interpreter)

        # We won't get here unless a version was found, because of the REQUIRED.
        set(QT_INTERNAL_SBOM_PYTHON_EXECUTABLE "${Python3_EXECUTABLE}" CACHE STRING
            "Python interpeter used for SBOM steps")
    endif()
endfunction()

# Helper to find the various python package dependencies needed to run the post-installation NTIA
# verification and the spdx format validation step.
function(_qt_internal_sbom_find_python_dependencies)
    if(NOT QT_INTERNAL_SBOM_PYTHON_EXECUTABLE)
        message(FATAL_ERROR "Python interpreter not found for sbom dependencies.")
    endif()

    if(QT_SBOM_HAVE_PYTHON_DEPS)
        return()
    endif()
    execute_process(
        COMMAND
            ${QT_INTERNAL_SBOM_PYTHON_EXECUTABLE} -c "
import spdx_tools.spdx.clitools.pyspdxtools
import ntia_conformance_checker.main
"
        RESULT_VARIABLE res
        OUTPUT_VARIABLE output
        ERROR_VARIABLE output
    )

    if("${res}" STREQUAL "0")
        set(QT_SBOM_HAVE_PYTHON_DEPS TRUE CACHE INTERNAL "")
    else()
        message(FATAL_ERROR "SBOM Python dependencies not found. Error:\n${output}")
    endif()
endfunction()

# Helper to find a python installed CLI utility.
# Expected to be in PATH.
function(_qt_internal_sbom_find_python_dependency_program)
    set(opt_args
        REQUIRED
    )
    set(single_args
        NAME
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    set(program_name "${arg_NAME}")
    string(TOUPPER "${program_name}" upper_name)
    set(cache_var "QT_SBOM_PROGRAM_${upper_name}")

    set(hints "")

    # The path to python installed apps is different on Windows compared to UNIX, so we use
    # a different path than where the python interpreter might be located.
    if(QT_SBOM_PYTHON_APPS_PATH)
        list(APPEND hints ${QT_SBOM_PYTHON_APPS_PATH})
    endif()

    find_program(${cache_var}
        NAMES ${program_name}
        HINTS ${hints}
    )

    if(NOT ${cache_var})
        if(arg_REQUIRED)
            set(message_type "FATAL_ERROR")
            set(prefix "Required ")
        else()
            set(message_type "STATUS")
            set(prefix "Optional ")
        endif()
        message(${message_type} "${prefix}SBOM python program '${program_name}' not found.")
    endif()
endfunction()

# Helper to generate a json file. This also implies some additional validity checks, useful
# to ensure a proper sbom file.
function(_qt_internal_sbom_generate_json)
    if(NOT QT_INTERNAL_SBOM_PYTHON_EXECUTABLE)
        message(FATAL_ERROR "Python interpreter not found for generating SBOM json file.")
    endif()

    set(content "
        message(STATUS \"Generating JSON: \${QT_SBOM_OUTPUT_PATH}.json\")
        execute_process(
            COMMAND ${QT_INTERNAL_SBOM_PYTHON_EXECUTABLE} -m spdx_tools.spdx.clitools.pyspdxtools
            -i \"\${QT_SBOM_OUTPUT_PATH}\" -o \"\${QT_SBOM_OUTPUT_PATH}.json\"
            RESULT_VARIABLE res
        )
        if(NOT res EQUAL 0)
            message(FATAL_ERROR \"SBOM conversion to JSON failed: \${res}\")
        endif()
")

    _qt_internal_get_current_project_sbom_dir(sbom_dir)
    set(verify_sbom "${sbom_dir}/convert_to_json.cmake")
    file(GENERATE OUTPUT "${verify_sbom}" CONTENT "${content}")

    set_property(GLOBAL APPEND PROPERTY _qt_sbom_cmake_verify_include_files "${verify_sbom}")
endfunction()

# Helper to verify the generated sbom is valid and NTIA compliant.
function(_qt_internal_sbom_verify_valid_and_ntia_compliant)
    if(NOT QT_INTERNAL_SBOM_PYTHON_EXECUTABLE)
        message(FATAL_ERROR "Python interpreter not found for verifying SBOM file.")
    endif()

    set(content "
        message(STATUS \"Verifying: \${QT_SBOM_OUTPUT_PATH}\")
        execute_process(
            COMMAND ${QT_INTERNAL_SBOM_PYTHON_EXECUTABLE} -m spdx_tools.spdx.clitools.pyspdxtools
            -i \"\${QT_SBOM_OUTPUT_PATH}\"
            RESULT_VARIABLE res
        )
        if(NOT res EQUAL 0)
            message(FATAL_ERROR \"SBOM verification failed: \${res}\")
        endif()

        execute_process(
            COMMAND ${QT_INTERNAL_SBOM_PYTHON_EXECUTABLE} -m ntia_conformance_checker.main
            --file \"\${QT_SBOM_OUTPUT_PATH}\"
            RESULT_VARIABLE res
        )
        if(NOT res EQUAL 0)
            message(FATAL_ERROR \"SBOM NTIA verification failed: \{res}\")
        endif()
")

    _qt_internal_get_current_project_sbom_dir(sbom_dir)
    set(verify_sbom "${sbom_dir}/verify_valid_and_ntia.cmake")
    file(GENERATE OUTPUT "${verify_sbom}" CONTENT "${content}")

    set_property(GLOBAL APPEND PROPERTY _qt_sbom_cmake_verify_include_files "${verify_sbom}")
endfunction()

# Helper to show the main sbom document info in the form of a CLI table.
function(_qt_internal_sbom_show_table)
    set(extra_code_begin "")
    if(DEFINED ENV{COIN_UNIQUE_JOB_ID})
        # The output of the process dynamically adjusts the width of the shown table based on the
        # console width. In the CI, the width is very short for some reason, and thus the output
        # is truncated in the CI log. Explicitly set a bigger width to avoid this.
        set(extra_code_begin "
set(backup_env_columns \$ENV{COLUMNS})
set(ENV{COLUMNS} 150)
")
set(extra_code_end "
set(ENV{COLUMNS} \${backup_env_columns})
")
    endif()

    set(content "
        message(STATUS \"Showing main SBOM document info: \${QT_SBOM_OUTPUT_PATH}\")

        ${extra_code_begin}
        execute_process(
            COMMAND ${QT_SBOM_PROGRAM_SBOM2DOC} -i \"\${QT_SBOM_OUTPUT_PATH}\"
            RESULT_VARIABLE res
        )
        ${extra_code_end}
        if(NOT res EQUAL 0)
            message(FATAL_ERROR \"Showing SBOM document failed: \${res}\")
        endif()
")

    _qt_internal_get_current_project_sbom_dir(sbom_dir)
    set(verify_sbom "${sbom_dir}/show_table.cmake")
    file(GENERATE OUTPUT "${verify_sbom}" CONTENT "${content}")

    set_property(GLOBAL APPEND PROPERTY _qt_sbom_cmake_verify_include_files "${verify_sbom}")
endfunction()

# Helper to audit the generated sbom.
function(_qt_internal_sbom_audit)
    set(opt_args NO_ERROR)
    set(single_args "")
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    set(handle_error "")
    if(NOT arg_NO_ERROR)
        set(handle_error "
            if(NOT res EQUAL 0)
                message(FATAL_ERROR \"SBOM Audit failed: \${res}\")
            endif()
")
    endif()

    set(content "
        message(STATUS \"Auditing SBOM: \${QT_SBOM_OUTPUT_PATH}\")
        execute_process(
            COMMAND ${QT_SBOM_PROGRAM_SBOMAUDIT} -i \"\${QT_SBOM_OUTPUT_PATH}\"
                    --disable-license-check --cpecheck --offline
            RESULT_VARIABLE res
        )
        ${handle_error}
")

    _qt_internal_get_current_project_sbom_dir(sbom_dir)
    set(verify_sbom "${sbom_dir}/audit.cmake")
    file(GENERATE OUTPUT "${verify_sbom}" CONTENT "${content}")

    set_property(GLOBAL APPEND PROPERTY _qt_sbom_cmake_verify_include_files "${verify_sbom}")
endfunction()

# Returns path to project's potential root source reuse.toml file.
function(_qt_internal_sbom_get_project_reuse_toml_path out_var)
    set(reuse_toml_path "${PROJECT_SOURCE_DIR}/REUSE.toml")
    set(${out_var} "${reuse_toml_path}" PARENT_SCOPE)
endfunction()

# Helper to generate and install a source SBOM using reuse.
function(_qt_internal_sbom_generate_reuse_source_sbom)
    set(opt_args NO_ERROR)
    set(single_args "")
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    _qt_internal_get_current_project_sbom_dir(sbom_dir)
    set(file_op "${sbom_dir}/generate_reuse_source_sbom.cmake")

    _qt_internal_sbom_get_project_reuse_toml_path(reuse_toml_path)
    if(NOT EXISTS "${reuse_toml_path}" AND NOT QT_FORCE_SOURCE_SBOM_GENERATION)
        set(skip_message
            "Skipping source SBOM generation: No reuse.toml file found at '${reuse_toml_path}'.")
        message(STATUS "${skip_message}")

        set(content "
            message(STATUS \"${skip_message}\")
")

        file(GENERATE OUTPUT "${file_op}" CONTENT "${content}")
        set_property(GLOBAL APPEND PROPERTY _qt_sbom_cmake_post_generation_include_files
            "${file_op}")
        return()
    endif()

    set(handle_error "")
    if(NOT arg_NO_ERROR)
        set(handle_error "
            if(NOT res EQUAL 0)
                message(FATAL_ERROR \"Source SBOM generation using reuse tool failed: \${res}\")
            endif()
")
    endif()

    set(source_sbom_path "\${QT_SBOM_OUTPUT_PATH_WITHOUT_EXT}.source.spdx")

    set(content "
        message(STATUS \"Generating source SBOM using reuse tool: ${source_sbom_path}\")
        execute_process(
            COMMAND ${QT_SBOM_PROGRAM_REUSE} --root \"${PROJECT_SOURCE_DIR}\" spdx
                    -o ${source_sbom_path}
            RESULT_VARIABLE res
        )
        ${handle_error}
")

    file(GENERATE OUTPUT "${file_op}" CONTENT "${content}")

    set_property(GLOBAL APPEND PROPERTY _qt_sbom_cmake_post_generation_include_files "${file_op}")
endfunction()

# Helper to run 'reuse lint' on the project source dir.
function(_qt_internal_sbom_run_reuse_lint)
    set(opt_args
        NO_ERROR
    )
    set(single_args
        BUILD_TIME_SCRIPT_PATH_OUT_VAR
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    # If no reuse.toml file exists, it means the repo is likely not reuse compliant yet,
    # so we shouldn't error out during installation when running the lint.
    _qt_internal_sbom_get_project_reuse_toml_path(reuse_toml_path)
    if(NOT EXISTS "${reuse_toml_path}" AND NOT QT_FORCE_REUSE_LINT_ERROR)
        set(arg_NO_ERROR TRUE)
    endif()

    set(handle_error "")
    if(NOT arg_NO_ERROR)
        set(handle_error "
            if(NOT res EQUAL 0)
                message(FATAL_ERROR \"Running 'reuse lint' failed: \${res}\")
            endif()
")
    endif()

    set(content "
        message(STATUS \"Running 'reuse lint' in '${PROJECT_SOURCE_DIR}'.\")
        execute_process(
            COMMAND ${QT_SBOM_PROGRAM_REUSE} --root \"${PROJECT_SOURCE_DIR}\" lint
            RESULT_VARIABLE res
        )
        ${handle_error}
")

    _qt_internal_get_current_project_sbom_dir(sbom_dir)
    set(file_op_build "${sbom_dir}/run_reuse_lint_build.cmake")
    file(GENERATE OUTPUT "${file_op_build}" CONTENT "${content}")

    # Allow skipping running 'reuse lint' during installation. But still allow running it during
    # build time. This is a fail safe opt-out in case some repo needs it.
    if(QT_FORCE_SKIP_REUSE_LINT_ON_INSTALL)
        set(skip_message "Skipping running 'reuse lint' in '${PROJECT_SOURCE_DIR}'.")

        set(content "
            message(STATUS \"${skip_message}\")
")
        set(file_op_install "${sbom_dir}/run_reuse_lint_install.cmake")
        file(GENERATE OUTPUT "${file_op_install}" CONTENT "${content}")
    else()
        # Just reuse the already generated script for installation as well.
        set(file_op_install "${file_op_build}")
    endif()

    set_property(GLOBAL APPEND PROPERTY _qt_sbom_cmake_verify_include_files "${file_op_install}")

    if(arg_BUILD_TIME_SCRIPT_PATH_OUT_VAR)
        set(${arg_BUILD_TIME_SCRIPT_PATH_OUT_VAR} "${file_op_build}" PARENT_SCOPE)
    endif()
endfunction()
