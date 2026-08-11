# To avoid CMP0057 if(IN_LIST) warnings
cmake_minimum_required(VERSION 3.16)

include(QtRunCMake)

function(run_cmake_and_build case format_case)
    set(opt_args "")
    set(single_args "")
    set(multi_args
        SEARCH_CASE_PACKAGES
    )
    cmake_parse_arguments(PARSE_ARGV 2 arg "${opt_args}" "${single_args}" "${multi_args}")

    set(include_file "${case}")
    set(original_case "${case}")
    set(case "${format_case}-${case}")

    # Set common build directory for configure and build
    set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/${case}-build)
    set(options
        "-DQt6_DIR=${Qt6_DIR}"
        "-DCMAKE_INSTALL_PREFIX=${RunCMake_TEST_BINARY_DIR}/installed"
        "-DSBOM_INCLUDE_FILE=${include_file}"
        "-DFORMAT_CASE=${format_case}"
    )

    set(extra_install_prefixes "")
    foreach(search_case IN LISTS arg_SEARCH_CASE_PACKAGES)
        set(case_install_prefix
            "${RunCMake_BINARY_DIR}/${format_case}-${search_case}-build/installed")
        list(APPEND extra_install_prefixes "${case_install_prefix}")
    endforeach()

    if (extra_install_prefixes)
        list(APPEND options "-DCMAKE_PREFIX_PATH=${extra_install_prefixes}")
    endif()

    # Check CI environment variables for SBOM options to ensure we only enabled checks that
    # require additional dependencies on machines that actually have them.
    # Also allow force enabling all checks via QT_SBOM_FORCE_ALL_CHECKS env var.
    set(maybe_sbom_env_args "$ENV{SBOM_COMMON_ARGS}")
    set(force_all_checks "$ENV{QT_SBOM_FORCE_ALL_CHECKS}")

    if(maybe_sbom_env_args MATCHES "QT_INTERNAL_SBOM_DEFAULT_CHECKS=ON"
            OR force_all_checks)
        list(APPEND options "-DQT_INTERNAL_SBOM_DEFAULT_CHECKS=ON")
    endif()

    if(maybe_sbom_env_args MATCHES "QT_INTERNAL_SBOM_AUDIT=ON"
            OR force_all_checks)
        list(APPEND options "-DQT_INTERNAL_SBOM_AUDIT=ON")
    endif()

    if(maybe_sbom_env_args MATCHES "QT_INTERNAL_SBOM_AUDIT_NO_ERROR=ON"
            OR force_all_checks)
        list(APPEND options "-DQT_INTERNAL_SBOM_AUDIT_NO_ERROR=ON")
    endif()

    set(require_spdx_json FALSE)
    if(maybe_sbom_env_args MATCHES "QT_SBOM_REQUIRE_GENERATE_SPDX_V2_JSON=ON" OR force_all_checks)
        set(require_spdx_json TRUE)
    endif()

    set(require_cydx FALSE)
    if(maybe_sbom_env_args MATCHES "QT_SBOM_REQUIRE_GENERATE_CYDX_V1_6=ON" OR force_all_checks)
        set(require_cydx TRUE)
    endif()

    if(format_case STREQUAL "spdx23")
        list(APPEND options
            -DQT_GENERATE_SBOM=ON
            -DEXPECTED_QT_GENERATE_SBOM=ON

            # Don't explicitly enable QT_SBOM_GENERATE_SPDX_V2 and QT_SBOM_GENERATE_SPDX_V2_JSON,
            # The are implicitly ON.
            # Make sure the tag value format is always generated.
            -DEXPECTED_QT_SBOM_GENERATE_SPDX_V2=ON

            # Explicitly disable CYDX though.
            -DQT_SBOM_GENERATE_CYDX_V1_6=OFF
            -DEXPECTED_QT_SBOM_GENERATE_CYDX_V1_6=OFF
        )
        if(require_spdx_json)
            # Require spdx json because because we assume we have the required dependencies from
            # reading the env var.
            list(APPEND options
                -DQT_SBOM_REQUIRE_GENERATE_SPDX_V2_JSON=ON
                -DEXPECTED_QT_SBOM_GENERATE_SPDX_V2_JSON=ON
            )
        endif()
    elseif(format_case STREQUAL "cydx16")
        list(APPEND options
            -DQT_GENERATE_SBOM=ON

            # Don't explicitly enable QT_SBOM_GENERATE_CYDX_V1_6.
            # It is implicitly ON.

            # Explicitly disable SPDX though.
            -DQT_SBOM_GENERATE_SPDX_V2=OFF
            -DEXPECTED_QT_SBOM_GENERATE_SPDX_V2=OFF
            -DQT_SBOM_GENERATE_SPDX_V2_JSON=OFF
            -DEXPECTED_QT_SBOM_GENERATE_SPDX_V2_JSON=OFF
        )
        if(require_cydx)
            list(APPEND options
                # This case is different from the others because if cydx deps are not found,
                # we will end up disabling both CYDX and GENERATE_SBOM, so we can only expect
                # this to be ON when we require cydx and the deps are found.
                # The assymetry is not very intuitive, but changing the behavior is too late.
                -DEXPECTED_QT_GENERATE_SBOM=ON

                # Require cydx because because we assume we have the required dependencies from
                # reading the env var.
                -DQT_SBOM_REQUIRE_GENERATE_CYDX_V1_6=ON
                -DEXPECTED_QT_SBOM_GENERATE_CYDX_V1_6=ON
            )
        endif()
    elseif(format_case STREQUAL "all")
        list(APPEND options
            -DQT_GENERATE_SBOM=ON
            -DEXPECTED_QT_GENERATE_SBOM=ON

            # Don't explicitly enable neither spdx nor cydx, they are both implicitly ON.
            # Make sure the tag value format is always generated.
            -DEXPECTED_QT_SBOM_GENERATE_SPDX_V2=ON
        )
        if(require_spdx_json)
            # Require spdx json because because we assume we have the required dependencies from
            # reading the env var.
            list(APPEND options
                -DQT_SBOM_REQUIRE_GENERATE_SPDX_V2_JSON=ON
                -DEXPECTED_QT_SBOM_GENERATE_SPDX_V2_JSON=ON
            )
        endif()
        if(require_cydx)
            # Require cydx because because we assume we have the required dependencies from
            # reading the env var.
            list(APPEND options
                -DQT_SBOM_REQUIRE_GENERATE_CYDX_V1_6=ON
                -DEXPECTED_QT_SBOM_GENERATE_CYDX_V1_6=ON
            )
        endif()
    elseif(format_case STREQUAL "none")
        list(APPEND options
            -DQT_GENERATE_SBOM=OFF
            -DEXPECTED_QT_GENERATE_SBOM=OFF

            # Confirm that the other values are implicitly ON, even though they won't be
            # generated.
            -DEXPECTED_QT_SBOM_GENERATE_SPDX_V2_JSON=ON
            -DEXPECTED_QT_SBOM_GENERATE_CYDX_V1_6=ON
        )
    endif()

    # Need to pass the python interpreter paths, to avoid sbom2doc not found errors.
    # This mirrors what coin/instructions/prepare_building_env.yaml does.
    set(maybe_python3_path "$ENV{SBOM_PYTHON_INTERP_PATH}")
    if(maybe_python3_path)
        list(APPEND options "-DQT_SBOM_PYTHON_INTERP=${maybe_python3_path}")
    endif()

    set(maybe_sbom_python_apps_path "$ENV{SBOM_PYTHON_APPS_PATH}")
    if(maybe_sbom_python_apps_path)
        list(APPEND options "-DQT_SBOM_PYTHON_APPS_PATH=${maybe_sbom_python_apps_path}")
    endif()

    run_cmake_with_options(${case} ${options})

    # Do not remove the current RunCMake_TEST_BINARY_DIR
    set(RunCMake_TEST_NO_CLEAN 1)
    # Merge output, because some of the spdx tooling outputs to stderr even when everything is
    # fine.
    set(RunCMake_TEST_OUTPUT_MERGE 1)
    run_cmake_command(${case}-build ${CMAKE_COMMAND} --build .)

    # Check the sbom files are present after installation. Use generic check.cmake, unless there's
    # a case-specific one.
    set(rel_case_specific_check "${original_case}-install-check.cmake")
    set(abs_case_specific_check "${CMAKE_CURRENT_LIST_DIR}/${rel_case_specific_check}")
    if(EXISTS "${abs_case_specific_check}")
        set(RunCMake-check-file "${rel_case_specific_check}")
    else()
        set(RunCMake-check-file "check.cmake")
    endif()

    run_cmake_command(${case}-install ${CMAKE_COMMAND} --install .)
    unset(RunCMake-check-file)
endfunction()

set(format_cases spdx23 cydx16 all none)
foreach(format_case IN LISTS format_cases)
    run_cmake_and_build(minimal "${format_case}")
    run_cmake_and_build(full "${format_case}")
    run_cmake_and_build(versions "${format_case}")
    run_cmake_and_build(entity_types "${format_case}")
    run_cmake_and_build(target_relationships "${format_case}")

    # The next test depends on the previous one successfully passing.
    run_cmake_and_build(target_relationships_external "${format_case}"
        SEARCH_CASE_PACKAGES target_relationships)

    run_cmake_and_build(project_relationships "${format_case}")
    run_cmake_and_build(spdx_suffixes "${format_case}")

    # The next test depends on the previous one successfully passing.
    run_cmake_and_build(spdx_suffixes_external "${format_case}"
        SEARCH_CASE_PACKAGES spdx_suffixes)

    run_cmake_and_build(attribution_files "${format_case}")
    run_cmake_and_build(build_tools "${format_case}")
    run_cmake_and_build(recursive_file_inclusion "${format_case}")
    run_cmake_and_build(multiple_project_calls_same_doc "${format_case}")
endforeach()

