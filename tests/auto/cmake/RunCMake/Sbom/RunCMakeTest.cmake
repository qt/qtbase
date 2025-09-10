include(QtRunCMake)

function(run_cmake_and_build case)
    # Set common build directory for configure and build
    set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/${case}-build)
    set(options
        "-DQt6_DIR=${Qt6_DIR}"
        "-DCMAKE_INSTALL_PREFIX=${RunCMake_TEST_BINARY_DIR}/installed"
    )

    set(maybe_sbom_env_args "$ENV{SBOM_COMMON_ARGS}")

    if(maybe_sbom_env_args MATCHES "QT_INTERNAL_SBOM_DEFAULT_CHECKS=ON")
        list(APPEND options "-DQT_INTERNAL_SBOM_DEFAULT_CHECKS=ON")
    endif()

    if(maybe_sbom_env_args MATCHES "QT_INTERNAL_SBOM_AUDIT=ON")
        list(APPEND options "-DQT_INTERNAL_SBOM_AUDIT=ON")
    endif()

    if(maybe_sbom_env_args MATCHES "QT_INTERNAL_SBOM_AUDIT_NO_ERROR=ON")
        list(APPEND options "-DQT_INTERNAL_SBOM_AUDIT_NO_ERROR=ON")
    endif()

    # Need to pass the python interpreter paths, to avoid sbom2doc not found errors.
    # This mirrors what coin/instructions/prepare_building_env.yaml does.
    set(maybe_python3_path "$ENV{PYTHON3_PATH}")
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
    run_cmake_command(${case}-install ${CMAKE_COMMAND} --install .)
endfunction()

run_cmake_and_build(minimal)
run_cmake_and_build(full)
run_cmake_and_build(versions)
