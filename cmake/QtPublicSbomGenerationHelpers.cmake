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

# Helper that returns the relative sbom build dir.
# To accommodate multiple projects within a qt repo (like qtwebengine), we need to choose separate
# build dirs for each project.
function(_qt_internal_get_current_project_sbom_relative_dir out_var)
    _qt_internal_sbom_get_root_project_name_lower_case(repo_project_name_lowercase)
    _qt_internal_sbom_get_qt_repo_project_name_lower_case(real_qt_repo_project_name_lowercase)

    if(repo_project_name_lowercase STREQUAL real_qt_repo_project_name_lowercase)
        set(sbom_dir "qt_sbom")
    else()
        set(sbom_dir "qt_sbom/${repo_project_name_lowercase}")
    endif()
    set(${out_var} "${sbom_dir}" PARENT_SCOPE)
endfunction()

# Helper that returns the directory where the intermediate sbom files will be generated.
function(_qt_internal_get_current_project_sbom_dir out_var)
    _qt_internal_get_current_project_sbom_relative_dir(relative_dir)
    set(sbom_dir "${PROJECT_BINARY_DIR}/${relative_dir}")
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
        PROJECT_COMMENT
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
        "\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/${default_sbom_file_name}")

    qt_internal_sbom_set_default_option_value(OUTPUT "${default_install_sbom_path}")
    qt_internal_sbom_set_default_option_value(OUTPUT_RELATIVE_PATH
        "${default_sbom_file_name}")

    qt_internal_sbom_set_default_option_value(LICENSE "NOASSERTION")
    qt_internal_sbom_set_default_option_value(PROJECT_FOR_SPDX_ID "Package-${arg_PROJECT}")
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

    set(cmake_version "Built by CMake ${CMAKE_VERSION}")
    set(system_name_and_processor "${CMAKE_SYSTEM_NAME} (${CMAKE_SYSTEM_PROCESSOR})")
    set(default_project_comment
        "${cmake_version} with ${cmake_configs} configuration for ${system_name_and_processor}")

    set(project_comment "${default_project_comment}")

    if(arg_PROJECT_COMMENT)
        string(APPEND project_comment "${arg_PROJECT_COMMENT}")
    endif()

    set(project_comment "<text>${project_comment}</text>")

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
RelationshipComment: <text>${project_spdx_id} is built by compiler ${CMAKE_CXX_COMPILER_ID} version ${CMAKE_CXX_COMPILER_VERSION}</text>

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
PackageComment: ${project_comment}
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
    set_property(GLOBAL PROPERTY _qt_sbom_project_spdx_id "${project_spdx_id}")

    set_property(GLOBAL PROPERTY _qt_sbom_build_output_path "${build_sbom_path}")
    set_property(GLOBAL PROPERTY _qt_sbom_build_output_path_without_ext
        "${build_sbom_path_without_ext}")

    set_property(GLOBAL PROPERTY _qt_sbom_install_output_path "${install_sbom_path}")
    set_property(GLOBAL PROPERTY _qt_sbom_install_output_path_without_ext
        "${install_sbom_path_without_ext}")

    set_property(GLOBAL APPEND PROPERTY _qt_sbom_cmake_include_files "${create_staging_file}")

    set_property(GLOBAL PROPERTY _qt_sbom_spdx_id_count 0)
endfunction()

# Handles the look up of Python, Python spdx dependencies and other various post-installation steps
# like NTIA validation, auditing, json generation, etc.
function(_qt_internal_sbom_setup_project_ops_generation)
    set(opt_args
        GENERATE_JSON
        GENERATE_JSON_REQUIRED
        GENERATE_SOURCE_SBOM
        VERIFY_SBOM
        VERIFY_SBOM_REQUIRED
        VERIFY_NTIA_COMPLIANT
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

    if(arg_GENERATE_JSON AND NOT QT_INTERNAL_NO_SBOM_PYTHON_OPS)
        set(op_args
            OP_KEY "GENERATE_JSON"
            OUT_VAR_DEPS_FOUND deps_found
        )
        if(arg_GENERATE_JSON_REQUIRED)
            list(APPEND op_args REQUIRED)
        endif()

        _qt_internal_sbom_find_and_handle_sbom_op_dependencies(${op_args})
        if(deps_found)
            _qt_internal_sbom_generate_json()
        endif()
    endif()

    if(arg_VERIFY_SBOM AND NOT QT_INTERNAL_NO_SBOM_PYTHON_OPS)
        set(op_args
            OP_KEY "VERIFY_SBOM"
            OUT_VAR_DEPS_FOUND deps_found
        )
        if(arg_VERIFY_SBOM_REQUIRED)
            list(APPEND op_args REQUIRED)
        endif()

        _qt_internal_sbom_find_and_handle_sbom_op_dependencies(${op_args})
        if(deps_found)
            _qt_internal_sbom_verify_valid()
        endif()
    endif()

    if(arg_VERIFY_NTIA_COMPLIANT AND NOT QT_INTERNAL_NO_SBOM_PYTHON_OPS)
        _qt_internal_sbom_find_and_handle_sbom_op_dependencies(REQUIRED OP_KEY "RUN_NTIA")
        _qt_internal_sbom_verify_ntia_compliant()
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
endfunction()

# Signals the end of recording sbom information for a project.
# Creates an 'sbom' custom target to generate an incomplete sbom at build time (no checksums).
# Creates install rules to install a complete (with checksums) sbom.
function(_qt_internal_sbom_end_project_generate)
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

    _qt_internal_sbom_collect_cmake_include_files(includes
        JOIN_WITH_NEWLINES
        PROPERTIES _qt_sbom_cmake_include_files _qt_sbom_cmake_end_include_files
    )

    # Before checksum includes are included after the verification codes have been collected
    # and before their merged checksum(s) has been computed.
    _qt_internal_sbom_collect_cmake_include_files(before_checksum_includes
        JOIN_WITH_NEWLINES
        PROPERTIES _qt_sbom_cmake_before_checksum_include_files
    )

    # After checksum includes are included after the checksum has been computed and written to the
    # QT_SBOM_VERIFICATION_CODE variable.
    _qt_internal_sbom_collect_cmake_include_files(after_checksum_includes
        JOIN_WITH_NEWLINES
        PROPERTIES _qt_sbom_cmake_after_checksum_include_files
    )

    # Post generation includes are included for both build and install time sboms, after
    # sbom generation has finished.
    _qt_internal_sbom_collect_cmake_include_files(post_generation_includes
        JOIN_WITH_NEWLINES
        PROPERTIES _qt_sbom_cmake_post_generation_include_files
    )

    # Verification only makes sense on installation, where the checksums are present.
    _qt_internal_sbom_collect_cmake_include_files(verify_includes
        JOIN_WITH_NEWLINES
        PROPERTIES _qt_sbom_cmake_verify_include_files
    )

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
    _qt_internal_sbom_get_qt_repo_project_name_lower_case(real_qt_repo_project_name_lowercase)

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

    get_cmake_property(qt_repo_deps _qt_repo_deps_${real_qt_repo_project_name_lowercase})
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
            ${before_checksum_includes}
            list(SORT QT_SBOM_VERIFICATION_CODES)
            string(REPLACE \";\" \"\" QT_SBOM_VERIFICATION_CODES \"\${QT_SBOM_VERIFICATION_CODES}\")
            file(WRITE \"${sbom_dir}/verification.txt\" \"\${QT_SBOM_VERIFICATION_CODES}\")
            file(SHA1 \"${sbom_dir}/verification.txt\" QT_SBOM_VERIFICATION_CODE)
            ${after_checksum_includes}
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
    set_property(GLOBAL PROPERTY _qt_sbom_cmake_before_checksum_include_files "")
    set_property(GLOBAL PROPERTY _qt_sbom_cmake_after_checksum_include_files "")
    set_property(GLOBAL PROPERTY _qt_sbom_cmake_post_generation_include_files "")
    set_property(GLOBAL PROPERTY _qt_sbom_cmake_verify_include_files "")
endfunction()

# Gets a list of cmake include file paths, joins them as include() statements and returns the
# output.
function(_qt_internal_sbom_collect_cmake_include_files out_var)
    set(opt_args
        JOIN_WITH_NEWLINES
    )
    set(single_args "")
    set(multi_args
        PROPERTIES
    )
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_PROPERTIES)
        message(FATAL_ERROR "PROPERTIES is required")
    endif()

    set(includes "")

    foreach(property IN LISTS arg_PROPERTIES)
        get_cmake_property(include_files "${property}")

        if(include_files)
            foreach(include_file IN LISTS include_files)
                list(APPEND includes "include(\"${include_file}\")")
            endforeach()
        endif()
    endforeach()

    if(arg_JOIN_WITH_NEWLINES)
        list(JOIN includes "\n" includes)
    endif()

    set(${out_var} "${includes}" PARENT_SCOPE)
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

    set(check_option "")
    if(arg_SPDXID)
        set(check_option "CHECK" "${arg_SPDXID}")
    endif()

    _qt_internal_sbom_get_and_check_spdx_id(
        VARIABLE arg_SPDXID
        ${check_option}
        HINTS "SPDXRef-${arg_FILENAME}"
    )

    qt_internal_sbom_set_default_option_value(LICENSE "NOASSERTION")
    qt_internal_sbom_set_default_option_value(COPYRIGHT "NOASSERTION")

    get_property(sbom_project_spdx_id GLOBAL PROPERTY _qt_sbom_project_spdx_id)
    if(NOT sbom_project_spdx_id)
        message(FATAL_ERROR "Call _qt_internal_sbom_begin_project() first")
    endif()
    if(NOT arg_RELATIONSHIP)
        set(arg_RELATIONSHIP "${sbom_project_spdx_id} CONTAINS ${arg_SPDXID}")
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
        # The variable is escaped, so it is evaluated during cmake install time, so that the value
        # can be overridden with cmake --install . --prefix <path>.
        set(install_prefix "\${CMAKE_INSTALL_PREFIX}")
    endif()

    set(content "
        if(NOT EXISTS \$ENV{DESTDIR}${install_prefix}/${arg_FILENAME}
                AND NOT QT_SBOM_BUILD_TIME AND NOT QT_SBOM_FAKE_CHECKSUM)
            if(NOT ${arg_OPTIONAL})
                message(FATAL_ERROR \"Cannot find '${arg_FILENAME}' to compute its checksum. \"
                    \"Expected to find it at '\$ENV{DESTDIR}${install_prefix}/${arg_FILENAME}' \")
            endif()
        else()
            if(NOT QT_SBOM_BUILD_TIME)
                if(QT_SBOM_FAKE_CHECKSUM)
                    set(sha1 \"158942a783ee1095eafacaffd93de73edeadbeef\")
                else()
                    file(SHA1 \$ENV{DESTDIR}${install_prefix}/${arg_FILENAME} sha1)
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

# Helper to add a reference to an external SPDX document.
#
# EXTERNAL_DOCUMENT_SPDX_ID: The spdx id by which the external document should be referenced in
# the current project SPDX document. This semantically serves as a pointer to the external document
# URI.
# e.g. DocumentRef-qtbase.
#
# EXTERNAL_DOCUMENT_FILE_PATH: The relative file path of the external sbom document.
# e.g. "sbom/qtbase-6.9.0.spdx"
#
# Can contain generator expressions.
# The file path is searched for in the directories specified by the
# EXTERNAL_DOCUMENT_INSTALL_PREFIXES option during sbom generation, to compute the file checksum.
# The file path is NOT embedded into the current project spdx document.
# Only its spdx id and namespace is embedded. The namespace is extracted from the contents of the
# referenced file.
#
# EXTERNAL_DOCUMENT_INSTALL_PREFIXES: A list of directories where the external document file path
# is searched for. The first existing file is used. Additionally the following locations are
# searched:
# - QT6_INSTALL_PREFIX
# - QT_ADDITIONAL_PACKAGES_PREFIX_PATH
# - QT_ADDITIONAL_SBOM_DOCUMENT_PATHS
#
# EXTERNAL_PACKAGE_SPDX_ID: If set, and no RELATIONSHIP_STRING is provided, an automatic DEPENDS_ON
# relationship is added from the current project spdx id to the package identified by
# $EXTERNAL_DOCUMENT_SPDX_ID:$EXTERNAL_PACKAGE_SPDX_ID options.
# This is mostly a beginner convenience.
#
# RELATIONSHIP_STRING: If set, it is used as the relationship string to add to the current project
# relationships.
function(_qt_internal_sbom_generate_add_external_reference)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    set(opt_args "")
    set(single_args
        EXTERNAL_DOCUMENT_FILE_PATH
        EXTERNAL_DOCUMENT_SPDX_ID
        EXTERNAL_PACKAGE_SPDX_ID
        RELATIONSHIP_STRING
    )
    set(multi_args
        EXTERNAL_DOCUMENT_INSTALL_PREFIXES
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    qt_internal_sbom_set_default_option_value_and_error_if_empty(EXTERNAL_DOCUMENT_FILE_PATH "")

    if(NOT arg_EXTERNAL_DOCUMENT_SPDX_ID)
        get_property(spdx_id_count GLOBAL PROPERTY _qt_sbom_spdx_id_count)
        set(arg_EXTERNAL_DOCUMENT_SPDX_ID "DocumentRef-${spdx_id_count}")
        math(EXPR spdx_id_count "${spdx_id_count} + 1")
        set_property(GLOBAL PROPERTY _qt_sbom_spdx_id_count "${spdx_id_count}")
    elseif(NOT "${arg_EXTERNAL_DOCUMENT_SPDX_ID}" MATCHES
            "^DocumentRef-[a-zA-Z0-9]+[-a-zA-Z0-9]+$")
        message(FATAL_ERROR "Invalid DocumentRef \"${arg_EXTERNAL_DOCUMENT_SPDX_ID}\"")
    endif()

    get_property(sbom_project_spdx_id GLOBAL PROPERTY _qt_sbom_project_spdx_id)
    if(NOT sbom_project_spdx_id)
        message(FATAL_ERROR "Call _qt_internal_sbom_begin_project() first")
    endif()
    if(arg_RELATIONSHIP_STRING STREQUAL "")
        if(arg_EXTERNAL_PACKAGE_SPDX_ID)
            set(external_package "${arg_EXTERNAL_DOCUMENT_SPDX_ID}:${arg_EXTERNAL_PACKAGE_SPDX_ID}")
            set(arg_RELATIONSHIP_STRING
                "${sbom_project_spdx_id} DEPENDS_ON ${external_package}")
        endif()
    else()
        string(REPLACE
            "@QT_SBOM_LAST_SPDXID@" "${arg_EXTERNAL_DOCUMENT_SPDX_ID}"
            arg_RELATIONSHIP_STRING "${arg_RELATIONSHIP_STRING}")
    endif()

    _qt_internal_get_staging_area_spdx_file_path(staging_area_spdx_file)

    set(install_prefixes "")

    # Always append the install time install prefix.
    # The variable is escaped, so it is evaluated during cmake install time, so that the value
    # can be overridden with cmake --install . --prefix <path>.
    list(APPEND install_prefixes "\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}")

    if(arg_EXTERNAL_DOCUMENT_INSTALL_PREFIXES)
        list(APPEND install_prefixes ${arg_EXTERNAL_DOCUMENT_INSTALL_PREFIXES})
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
    if(arg_RELATIONSHIP_STRING)
        set(relationship_content "
        file(APPEND \"${staging_area_spdx_file}\"
    \"Relationship: ${arg_RELATIONSHIP_STRING}\")
")
    endif()

    # File path may not exist yet, and it could be a generator expression.
    set(content "
        set(relative_file_name \"${arg_EXTERNAL_DOCUMENT_FILE_PATH}\")
        set(document_dir_paths ${install_prefixes})
        list(JOIN document_dir_paths \"\\n\" document_dir_paths_per_line)
        foreach(document_dir_path IN LISTS document_dir_paths)
            set(document_file_path \"\${document_dir_path}/\${relative_file_name}\")
            if(EXISTS \"\${document_file_path}\")
                break()
            endif()
        endforeach()
        if(NOT EXISTS \"\${document_file_path}\")
            message(FATAL_ERROR \"Could not find external SBOM document \${relative_file_name}\"
                \" in any of the document dir paths: \${document_dir_paths_per_line} \"
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
ExternalDocumentRef: ${arg_EXTERNAL_DOCUMENT_SPDX_ID} \${ext_ns} SHA1: \${ext_sha1}\")

        ${relationship_content}
")

    _qt_internal_get_current_project_sbom_dir(sbom_dir)
    set(ext_ref_sbom "${sbom_dir}/${arg_EXTERNAL_DOCUMENT_SPDX_ID}.cmake")
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

    set(check_option "")
    if(arg_SPDXID)
        set(check_option "CHECK" "${arg_SPDXID}")
    endif()

    _qt_internal_sbom_get_and_check_spdx_id(
        VARIABLE arg_SPDXID
        ${check_option}
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

    get_property(sbom_project_spdx_id GLOBAL PROPERTY _qt_sbom_project_spdx_id)
    if(NOT sbom_project_spdx_id)
        message(FATAL_ERROR "Call _qt_internal_sbom_begin_project() first")
    endif()
    if(NOT arg_RELATIONSHIP)
        set(arg_RELATIONSHIP "${sbom_project_spdx_id} CONTAINS ${arg_SPDXID}")
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

# Adds a cmake include file to the sbom generation process at a specific step.
# INCLUDE_PATH - path to the cmake file to include.
# STEP - one of
#        BEGIN
#        END
#        POST_GENERATION
#        VERIFY
#
# BEGIN includes are included after the spdx staging file is created.
#
# END includes are included after the all the BEGIN ones are included.
#
# BEFORE_CHECKSUM includes are included after the verification codes have been collected
# and before their merged checksum(s) has been computed. Only relevant for install time sboms.
#
# AFTER_CHECKSUM includes are included after the checksum has been computed and written to the
# QT_SBOM_VERIFICATION_CODE variable. Only relevant for install time sboms.
#
# POST_GENERATION includes are included for both build and install time sboms, after
# sbom generation has finished.
# Currently used for adding reuse lint and reuse source steps, before VERIFY include are run.
#
# VERIFY includes only make sense on installation, where the checksums are present, so they are
# only included during install time.
# Used for generating a json sbom from the tag value one, for running ntia compliance check, etc.
function(_qt_internal_sbom_add_cmake_include_step)
    set(opt_args "")
    set(single_args
        STEP
        INCLUDE_PATH
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")

    if(NOT arg_STEP)
        message(FATAL_ERROR "STEP is required")
    endif()

    if(NOT arg_INCLUDE_PATH)
        message(FATAL_ERROR "INCLUDE_PATH is required")
    endif()

    set(step "${arg_STEP}")

    if(step STREQUAL "BEGIN")
        set(property "_qt_sbom_cmake_include_files")
    elseif(step STREQUAL "END")
        set(property "_qt_sbom_cmake_end_include_files")
    elseif(step STREQUAL "BEFORE_CHECKSUM")
        set(property "_qt_sbom_cmake_before_checksum_include_files")
    elseif(step STREQUAL "AFTER_CHECKSUM")
        set(property "_qt_sbom_cmake_after_checksum_include_files")
    elseif(step STREQUAL "POST_GENERATION")
        set(property "_qt_sbom_cmake_post_generation_include_files")
    elseif(step STREQUAL "VERIFY")
        set(property "_qt_sbom_cmake_verify_include_files")
    else()
        message(FATAL_ERROR "Invalid SBOM cmake include step name: ${step}")
    endif()

    set_property(GLOBAL APPEND PROPERTY "${property}" "${arg_INCLUDE_PATH}")
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

    set(check_option "")
    if(arg_SPDXID)
        set(check_option "CHECK" "${arg_SPDXID}")
    endif()

    _qt_internal_sbom_get_and_check_spdx_id(
        VARIABLE arg_SPDXID
        ${check_option}
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

# Helper to find a python interpreter and a specific python dependency, e.g. to be able to generate
# a SPDX JSON SBOM, or run post-installation steps like NTIA verification.
# The exact dependency should be specified as the OP_KEY.
#
# Caches the found python executable in a separate cache var QT_INTERNAL_SBOM_PYTHON_EXECUTABLE, to
# avoid conflicts with any other found python interpreter.
function(_qt_internal_sbom_find_and_handle_sbom_op_dependencies)
    set(opt_args
        REQUIRED
    )
    set(single_args
        OP_KEY
        OUT_VAR_DEPS_FOUND
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_OP_KEY)
        message(FATAL_ERROR "OP_KEY is required")
    endif()

    set(supported_ops "GENERATE_JSON" "VERIFY_SBOM" "RUN_NTIA")

    if(arg_OP_KEY STREQUAL "GENERATE_JSON" OR arg_OP_KEY STREQUAL "VERIFY_SBOM")
        set(import_statement "import spdx_tools.spdx.clitools.pyspdxtools")
    elseif(arg_OP_KEY STREQUAL "RUN_NTIA")
        set(import_statement "import ntia_conformance_checker.main")
    else()
        message(FATAL_ERROR "OP_KEY must be one of ${supported_ops}")
    endif()

    # Return early if we found the dependencies.
    if(QT_INTERNAL_SBOM_DEPS_FOUND_FOR_${arg_OP_KEY})
        if(arg_OUT_VAR_DEPS_FOUND)
            set(${arg_OUT_VAR_DEPS_FOUND} TRUE PARENT_SCOPE)
        endif()
        return()
    endif()

    # NTIA-compliance checker requires Python 3.9 or later, so we use it as the minimum for all
    # SBOM OPs.
    set(required_version "3.9")

    set(python_common_args
        VERSION "${required_version}"
    )

    set(everything_found FALSE)

    # On macOS FindPython prefers looking in the system framework location, but that usually would
    # not have the required dependencies. So we first look in it, and then fallback to any other
    # non-framework python found.
    if(CMAKE_HOST_APPLE)
        set(extra_python_args SEARCH_IN_FRAMEWORKS QUIET)
        _qt_internal_sbom_find_python_and_dependency_helper_lambda()
    endif()

    if(NOT everything_found)
        set(extra_python_args QUIET)
        _qt_internal_sbom_find_python_and_dependency_helper_lambda()
    endif()

    if(NOT everything_found)
        if(arg_REQUIRED)
            set(message_type "FATAL_ERROR")
        else()
            set(message_type "DEBUG")
        endif()

        if(NOT python_found)
            # Look for python one more time, this time without QUIET, to show an error why it
            # wasn't found.
            if(arg_REQUIRED)
                _qt_internal_sbom_find_python_helper(${python_common_args}
                    OUT_VAR_PYTHON_PATH unused_python
                    OUT_VAR_PYTHON_FOUND unused_found
                )
            endif()
            message(${message_type} "Python ${required_version} for running SBOM ops not found.")
        elseif(NOT dep_found)
            message(${message_type} "Python dependency for running SBOM op ${arg_OP_KEY} "
                "not found:\n Python: ${python_path} \n Output: \n${dep_find_output}")
        endif()
    else()
        message(DEBUG "Using Python ${python_path} for running SBOM ops.")

        if(NOT QT_INTERNAL_SBOM_PYTHON_EXECUTABLE)
            set(QT_INTERNAL_SBOM_PYTHON_EXECUTABLE "${python_path}" CACHE INTERNAL
                "Python interpeter used for SBOM generation.")
        endif()

        set(QT_INTERNAL_SBOM_DEPS_FOUND_FOR_${arg_OP_KEY} "TRUE" CACHE INTERNAL
            "All dependencies found to run SBOM OP ${arg_OP_KEY}")
    endif()

    if(arg_OUT_VAR_DEPS_FOUND)
        set(${arg_OUT_VAR_DEPS_FOUND} "${QT_INTERNAL_SBOM_DEPS_FOUND_FOR_${arg_OP_KEY}}"
            PARENT_SCOPE)
    endif()
endfunction()

# Helper macro to find python and a given dependency. Expects the caller to set all of the vars.
# Meant to reduce the line noise due to the repeated calls.
macro(_qt_internal_sbom_find_python_and_dependency_helper_lambda)
    _qt_internal_sbom_find_python_and_dependency_helper(
        PYTHON_ARGS
            ${extra_python_args}
            ${python_common_args}
        DEPENDENCY_ARGS
            DEPENDENCY_IMPORT_STATEMENT "${import_statement}"
        OUT_VAR_PYTHON_PATH python_path
        OUT_VAR_PYTHON_FOUND python_found
        OUT_VAR_DEP_FOUND dep_found
        OUT_VAR_PYTHON_AND_DEP_FOUND everything_found
        OUT_VAR_DEP_FIND_OUTPUT dep_find_output
    )
endmacro()

# Tries to find python and a given dependency based on the args passed to PYTHON_ARGS and
# DEPENDENCY_ARGS which are forwarded to the respective finding functions.
# Returns the path to the python interpreter, whether it was found, whether the dependency was
# found, whether both were found, and the reason why the dependency might not be found.
function(_qt_internal_sbom_find_python_and_dependency_helper)
    set(opt_args)
    set(single_args
        OUT_VAR_PYTHON_PATH
        OUT_VAR_PYTHON_FOUND
        OUT_VAR_DEP_FOUND
        OUT_VAR_PYTHON_AND_DEP_FOUND
        OUT_VAR_DEP_FIND_OUTPUT
    )
    set(multi_args
        PYTHON_ARGS
        DEPENDENCY_ARGS
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    set(everything_found_inner FALSE)
    set(deps_find_output_inner "")

    if(NOT arg_OUT_VAR_PYTHON_PATH)
        message(FATAL_ERROR "OUT_VAR_PYTHON_PATH var is required")
    endif()

    if(NOT arg_OUT_VAR_PYTHON_FOUND)
        message(FATAL_ERROR "OUT_VAR_PYTHON_FOUND var is required")
    endif()

    if(NOT arg_OUT_VAR_DEP_FOUND)
        message(FATAL_ERROR "OUT_VAR_DEP_FOUND var is required")
    endif()

    if(NOT arg_OUT_VAR_PYTHON_AND_DEP_FOUND)
        message(FATAL_ERROR "OUT_VAR_PYTHON_AND_DEP_FOUND var is required")
    endif()

    if(NOT arg_OUT_VAR_DEP_FIND_OUTPUT)
        message(FATAL_ERROR "OUT_VAR_DEP_FIND_OUTPUT var is required")
    endif()

    _qt_internal_sbom_find_python_helper(
        ${arg_PYTHON_ARGS}
        OUT_VAR_PYTHON_PATH python_path_inner
        OUT_VAR_PYTHON_FOUND python_found_inner
    )

    if(python_found_inner AND python_path_inner)
        _qt_internal_sbom_find_python_dependency_helper(
            ${arg_DEPENDENCY_ARGS}
            PYTHON_PATH "${python_path_inner}"
            OUT_VAR_FOUND dep_found_inner
            OUT_VAR_OUTPUT dep_find_output_inner
        )

        if(dep_found_inner)
            set(everything_found_inner TRUE)
        endif()
    endif()

    set(${arg_OUT_VAR_PYTHON_PATH} "${python_path_inner}" PARENT_SCOPE)
    set(${arg_OUT_VAR_PYTHON_FOUND} "${python_found_inner}" PARENT_SCOPE)
    set(${arg_OUT_VAR_DEP_FOUND} "${dep_found_inner}" PARENT_SCOPE)
    set(${arg_OUT_VAR_PYTHON_AND_DEP_FOUND} "${everything_found_inner}" PARENT_SCOPE)
    set(${arg_OUT_VAR_DEP_FIND_OUTPUT} "${dep_find_output_inner}" PARENT_SCOPE)
endfunction()

# Tries to find the python intrepreter, given the QT_SBOM_PYTHON_INTERP path hint, as well as
# other options.
# Ignores any previously found python.
# Returns the python interpreter path and whether it was successfully found.
#
# This is intentionally a function, and not a macro, to prevent overriding the Python3_EXECUTABLE
# non-cache variable in a global scope in case if a different python is found and used for a
# different purpose (e.g. qtwebengine or qtinterfaceframework).
# The reason to use a different python is that an already found python might not be the version we
# need, or might lack the dependencies we need.
# https://gitlab.kitware.com/cmake/cmake/-/issues/21797#note_901621 claims that finding multiple
# python versions in separate directory scopes is possible, and I claim a function scope is as
# good as a directory scope.
function(_qt_internal_sbom_find_python_helper)
    set(opt_args
        SEARCH_IN_FRAMEWORKS
        QUIET
    )
    set(single_args
        VERSION
        OUT_VAR_PYTHON_PATH
        OUT_VAR_PYTHON_FOUND
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_OUT_VAR_PYTHON_PATH)
        message(FATAL_ERROR "OUT_VAR_PYTHON_PATH var is required")
    endif()

    if(NOT arg_OUT_VAR_PYTHON_FOUND)
        message(FATAL_ERROR "OUT_VAR_PYTHON_FOUND var is required")
    endif()

    # Allow disabling looking for a python interpreter shipped as part of a macOS system framework.
    if(NOT arg_SEARCH_IN_FRAMEWORKS)
        set(Python3_FIND_FRAMEWORK NEVER)
    endif()

    set(required_version "")
    if(arg_VERSION)
        set(required_version "${arg_VERSION}")
    endif()

    set(find_quiet "")
    if(arg_QUIET)
        set(find_quiet "QUIET")
    endif()

    # Locally reset any executable that was possibly already found.
    # We do this to ensure we always re-do the lookup/
    # This needs to be set to an empty string, to override any cache variable
    set(Python3_EXECUTABLE "")

    # This needs to be unset, because the Python module checks whether the variable is defined, not
    # whether it is empty.
    unset(_Python3_EXECUTABLE)

    if(QT_SBOM_PYTHON_INTERP)
        set(Python3_ROOT_DIR ${QT_SBOM_PYTHON_INTERP})
    endif()

    find_package(Python3 ${required_version} ${find_quiet} COMPONENTS Interpreter)

    set(${arg_OUT_VAR_PYTHON_PATH} "${Python3_EXECUTABLE}" PARENT_SCOPE)
    set(${arg_OUT_VAR_PYTHON_FOUND} "${Python3_Interpreter_FOUND}" PARENT_SCOPE)
endfunction()

# Helper that takes an python import statement to run using the given python interpreter path,
# to confirm that the given python dependency can be found.
# Returns whether the dependency was found and the output of running the import, for error handling.
function(_qt_internal_sbom_find_python_dependency_helper)
    set(opt_args "")
    set(single_args
        DEPENDENCY_IMPORT_STATEMENT
        PYTHON_PATH
        OUT_VAR_FOUND
        OUT_VAR_OUTPUT
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_PYTHON_PATH)
        message(FATAL_ERROR "Python interpreter path not given.")
    endif()

    if(NOT arg_DEPENDENCY_IMPORT_STATEMENT)
        message(FATAL_ERROR "Python depdendency import statement not given.")
    endif()

    if(NOT arg_OUT_VAR_FOUND)
        message(FATAL_ERROR "Out var found variable not given.")
    endif()

    set(python_path "${arg_PYTHON_PATH}")
    execute_process(
        COMMAND
            ${python_path} -c "${arg_DEPENDENCY_IMPORT_STATEMENT}"
        RESULT_VARIABLE res
        OUTPUT_VARIABLE output
        ERROR_VARIABLE output
    )

    if("${res}" STREQUAL "0")
        set(found TRUE)
        set(output "${output}")
    else()
        set(found FALSE)
        string(CONCAT output "SBOM Python dependency ${arg_DEPENDENCY_IMPORT_STATEMENT} not found. "
            "Error:\n${output}")
    endif()

    set(${arg_OUT_VAR_FOUND} "${found}" PARENT_SCOPE)
    if(arg_OUT_VAR_OUTPUT)
        set(${arg_OUT_VAR_OUTPUT} "${output}" PARENT_SCOPE)
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
    if(NOT QT_INTERNAL_SBOM_DEPS_FOUND_FOR_GENERATE_JSON)
        message(FATAL_ERROR "Python dependencies not found for generating SBOM json file.")
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

# Helper to verify the generated sbom is valid.
function(_qt_internal_sbom_verify_valid)
    if(NOT QT_INTERNAL_SBOM_PYTHON_EXECUTABLE)
        message(FATAL_ERROR "Python interpreter not found for verifying SBOM file.")
    endif()

    if(NOT QT_INTERNAL_SBOM_DEPS_FOUND_FOR_VERIFY_SBOM)
        message(FATAL_ERROR "Python dependencies not found for verifying SBOM file")
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
")

    _qt_internal_get_current_project_sbom_dir(sbom_dir)
    set(verify_sbom "${sbom_dir}/verify_valid.cmake")
    file(GENERATE OUTPUT "${verify_sbom}" CONTENT "${content}")

    set_property(GLOBAL APPEND PROPERTY _qt_sbom_cmake_verify_include_files "${verify_sbom}")
endfunction()

# Helper to verify the generated sbom is NTIA compliant.
function(_qt_internal_sbom_verify_ntia_compliant)
    if(NOT QT_INTERNAL_SBOM_PYTHON_EXECUTABLE)
        message(FATAL_ERROR "Python interpreter not found for verifying SBOM file.")
    endif()

    if(NOT QT_INTERNAL_SBOM_DEPS_FOUND_FOR_RUN_NTIA)
        message(FATAL_ERROR "Python dependencies not found for running the SBOM NTIA checker.")
    endif()

    set(content "
        message(STATUS \"Checking for NTIA compliance: \${QT_SBOM_OUTPUT_PATH}\")
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
    set(verify_sbom "${sbom_dir}/verify_ntia.cmake")
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
