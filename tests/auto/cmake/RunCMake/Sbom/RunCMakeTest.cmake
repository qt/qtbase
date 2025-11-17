include(QtRunCMake)

function(run_cmake_and_build case format_case)
    set(include_file "${case}")
    set(case "${format_case}-${case}")

    # Set common build directory for configure and build
    set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/${case}-build)
    set(options
        "-DQt6_DIR=${Qt6_DIR}"
        "-DCMAKE_INSTALL_PREFIX=${RunCMake_TEST_BINARY_DIR}/installed"
        "-DSBOM_INCLUDE_FILE=${include_file}"
        "-DFORMAT_CASE=${format_case}"
    )

    if(format_case STREQUAL "spdx23")
        list(APPEND options
            -DQT_GENERATE_SBOM=ON
            -DQT_SBOM_GENERATE_SPDX_V2=ON
            -DQT_SBOM_GENERATE_CYDX_V1_6=OFF
        )
    elseif(format_case STREQUAL "cydx16")
        list(APPEND options
            -DQT_GENERATE_SBOM=ON
            -DQT_SBOM_GENERATE_SPDX_V2=OFF
            -DQT_SBOM_GENERATE_CYDX_V1_6=ON
        )
    elseif(format_case STREQUAL "all")
        list(APPEND options
            -DQT_GENERATE_SBOM=ON
            -DQT_SBOM_GENERATE_SPDX_V2=ON
            -DQT_SBOM_GENERATE_CYDX_V1_6=ON
        )
    elseif(format_case STREQUAL "none")
        list(APPEND options
            -DQT_GENERATE_SBOM=OFF
        )
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

    if(maybe_sbom_env_args MATCHES "QT_SBOM_REQUIRE_GENERATE_CYDX_V1_6=ON"
            OR force_all_checks)
        list(APPEND options "-DQT_SBOM_REQUIRE_GENERATE_CYDX_V1_6=ON")
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

    # Check the sbom files are present after installation.
    set(RunCMake-check-file "check.cmake")
    run_cmake_command(${case}-install ${CMAKE_COMMAND} --install .)
    unset(RunCMake-check-file)
endfunction()

set(format_cases spdx23 cydx16 all none)
foreach(format_case IN LISTS format_cases)
    run_cmake_and_build(minimal "${format_case}")
    run_cmake_and_build(full "${format_case}")
    run_cmake_and_build(versions "${format_case}")
endforeach()

