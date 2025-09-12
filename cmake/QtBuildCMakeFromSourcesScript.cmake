# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

# Query the var name from the CMake cache or the environment or use a default value.
function(qt_internal_get_cmake_or_env_or_default out_var var_name_to_check default_value)
    if(${var_name_to_check})
        set(value "${var_name_to_check}")
    elseif(DEFINED ENV{${var_name_to_check}})
        set(value "$ENV{${var_name_to_check}}")
    else()
        set(value "${default_value}")
    endif()
    set(${out_var} "${value}" PARENT_SCOPE)
endfunction()

# Prepares paths for the cloning of the sources, the build and the installation.
function(qt_internal_prepare_build_paths)
    set(current_dir "${CMAKE_CURRENT_BINARY_DIR}")

    qt_internal_get_cmake_or_env_or_default(working_dir
        QT_CI_CUSTOM_CMAKE_WORKING_DIR "${current_dir}/cmake_build")

    qt_internal_get_cmake_or_env_or_default(src_dir
        QT_CI_CUSTOM_CMAKE_SRC_DIR "${working_dir}/src")

    qt_internal_get_cmake_or_env_or_default(build_dir
        QT_CI_CUSTOM_CMAKE_BUILD_DIR "${working_dir}/build")

    qt_internal_get_cmake_or_env_or_default(installed_dir
        QT_CI_CUSTOM_CMAKE_INSTALL_DIR "${working_dir}/install")

    set(QT_CUSTOM_CMAKE_WORKING_DIR "${working_dir}" PARENT_SCOPE)
    set(QT_CUSTOM_CMAKE_SRC_DIR "${src_dir}" PARENT_SCOPE)
    set(QT_CUSTOM_CMAKE_BUILD_DIR "${build_dir}" PARENT_SCOPE)
    set(QT_CUSTOM_CMAKE_INSTALL_DIR "${installed_dir}" PARENT_SCOPE)
endfunction()

# Gets the remote git base URL for qt repositories.
function(qt_internal_get_repo_base_url out_var)

    # This Coin CI git daemon IP is set in all CI jobs.
    # Prefer using it when available, to avoid general network issues.
    # Sample value: QT_COIN_GIT_DAEMON=10.215.0.212:9418
    qt_internal_get_cmake_or_env_or_default(coin_git_ip_port QT_COIN_GIT_DAEMON "")

    # Allow opting out of using the coin git daemon.
    qt_internal_get_cmake_or_env_or_default(skip_coin_git
        QT_CI_CMAKE_BUILD_SKIP_COIN_GIT_DAEMON FALSE)

    # Allow override of the default remote.
    qt_internal_get_cmake_or_env_or_default(git_remote QT_CI_CMAKE_GIT_REMOTE "")

    if(coin_git_ip_port AND NOT skip_coin_git)
        set(value "git://${coin_git_ip_port}/qt-project")
    elseif(git_remote)
        set(value "${git_remote}")
    else()
        set(value "https://codereview.qt-project.org")
    endif()

    set(${out_var} "${value}" PARENT_SCOPE)
endfunction()

# Clones qt5.git into the specified src directory.
function(qt_internal_clone_cmake)
    file(MAKE_DIRECTORY "${QT_CUSTOM_CMAKE_SRC_DIR}")

    # Allow pinning the sha1 or any other git ref, based on a cmake var or env var.
    qt_internal_get_cmake_or_env_or_default(super_repo_ref
        QT_CI_CUSTOM_CMAKE_TOP_LEVEL_PIN_GIT_REF "upstream/master")

    qt_internal_get_repo_base_url(remote_base_url)
    set(remote_url "${remote_base_url}/kitware/cmake")

    execute_process(
        COMMAND
            git clone --progress --depth 2
            -b "${super_repo_ref}" --single-branch "${remote_url}" "${QT_CUSTOM_CMAKE_SRC_DIR}"
        COMMAND_ECHO STDOUT
        WORKING_DIRECTORY "${QT_CUSTOM_CMAKE_WORKING_DIR}"
        RESULT_VARIABLE result
    )
    if(result)
        message(WARNING "Cloning CMake sources failed. Output: ${result}")
    endif()


    execute_process(
        COMMAND git checkout "${super_repo_ref}"
        COMMAND_ECHO STDOUT
        WORKING_DIRECTORY "${QT_CUSTOM_CMAKE_SRC_DIR}"
        RESULT_VARIABLE result
    )
    if(result)
        message(FATAL_ERROR
            "Checking out CMake.git '${super_repo_ref}' ref failed. Output: ${result}")
    endif()
endfunction()

function(qt_internal_get_msvc_env_script out_var)
    set(default_value
        "C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Auxiliary/Build/vcvarsall.bat")

    qt_internal_get_cmake_or_env_or_default(msvc_env_script
        QT_CI_CUSTOM_CMAKE_MSVC_ENV_SCRIPT_PATH "${default_value}")

    file(TO_NATIVE_PATH "${msvc_env_script}" msvc_env_script)
    set(${out_var} "${msvc_env_script}" PARENT_SCOPE)
endfunction()

function(qt_internal_get_prefix_script out_var)
    if(NOT CMAKE_HOST_WIN32)
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    set(script_path "${QT_CUSTOM_CMAKE_BUILD_DIR}/prefix.bat")
    qt_internal_get_msvc_env_script(vc_script)

    # Determine architecture.
    # For some reason CMAKE_HOST_SYSTEM_PROCESSOR can report amd64 even on an arm64 host,
    # perhaps due to emulation, similar to rosetta on macOS.
    # So also check the PROCESSOR_ARCHITECTURE env var, which is the var CMake checks as well.
    set(maybe_win_proc_arch "$ENV{PROCESSOR_ARCHITECTURE}")
    if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(aarch64|ARM64|arm64)$"
            OR maybe_win_proc_arch MATCHES "^(aarch64|ARM64|arm64)$")
        set(arch "arm64")
    else()
        set(arch "amd64")
    endif()
    message(STATUS "CMAKE_HOST_SYSTEM_PROCESSOR: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
    message(STATUS "CMAKE_HOST_SYSTEM_VERSION: ${CMAKE_HOST_SYSTEM_VERSION}")
    message(STATUS "PROCESSOR_ARCHITECTURE: ${maybe_win_proc_arch}")
    message(STATUS "Chosen architecture for MSVC env script: ${arch}")

    set(content "call \"${vc_script}\" ${arch} \r\ncmd /c %*")

    file(WRITE "${script_path}" "${content}")
    message(STATUS "Generating prefix script at ${script_path} with content: '${content}'")

    set(value "${script_path}")
    set(${out_var} "${value}" PARENT_SCOPE)
endfunction()

# Configures CMake.
function(qt_internal_configure_cmake)
    file(MAKE_DIRECTORY "${QT_CUSTOM_CMAKE_BUILD_DIR}")
    file(MAKE_DIRECTORY "${QT_CUSTOM_CMAKE_INSTALL_DIR}")

    # Get the common CI args, to set the sccache args, etc.
    qt_internal_get_cmake_or_env_or_default(ci_common_cmake_args COMMON_CMAKE_ARGS "")
    if(ci_common_cmake_args)
        separate_arguments(ci_common_cmake_args NATIVE_COMMAND ${ci_common_cmake_args})
    endif()

    set(extra_cmake_args "")

    set(prefix_script "")
    if(CMAKE_HOST_WIN32)
        # Avoid errors when building with sccache and MSVC due to the /Zi flag, instead force use
        # /Z7.
        list(APPEND extra_cmake_args "-DCMAKE_MSVC_DEBUG_INFORMATION_FORMAT=Embedded")

        # Always build with MSVC, because clang / gcc might not be sufficient
        # (e.g. missing std::filesystem).
        list(APPEND extra_cmake_args "-DCMAKE_C_COMPILER=cl.exe")
        list(APPEND extra_cmake_args "-DCMAKE_CXX_COMPILER=cl.exe")

        qt_internal_get_prefix_script(prefix_script)
    endif()

    set(osx_arches "")
    if(CMAKE_HOST_APPLE)
        # Also set a low enough macOS deployment target
        list(APPEND extra_cmake_args "-DCMAKE_OSX_DEPLOYMENT_TARGET=13")

        # On macOS build both arches.
        # Use a separate variable to avoid the semicolon causing cmake to treat it as a
        # multi-value list
        set(osx_arches "-DCMAKE_OSX_ARCHITECTURES=x86_64\;arm64")
    endif()

    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24")
        list(APPEND extra_cmake_args "--fresh")
    endif()

    execute_process(
        COMMAND
            ${prefix_script}
            cmake
            -S "${QT_CUSTOM_CMAKE_SRC_DIR}"
            -B "${QT_CUSTOM_CMAKE_BUILD_DIR}"
            -DCMAKE_BUILD_TYPE=RelWithDebInfo
            "-DCMAKE_INSTALL_PREFIX=${QT_CUSTOM_CMAKE_INSTALL_DIR}"
            --log-level STATUS
            -GNinja
            ${ci_common_cmake_args}
            # Extra cmake args should go after the CI common ones, to allow overriding them.
            ${extra_cmake_args}
            ${osx_arches}

        COMMAND_ECHO STDOUT
        WORKING_DIRECTORY "${QT_CUSTOM_CMAKE_BUILD_DIR}"
        RESULT_VARIABLE result
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(result)
        message(FATAL_ERROR "Configuring cmake build failed. Output: ${result}")
    endif()
endfunction()

# Builds cmake.
function(qt_internal_build_cmake)
    set(prefix_script "")

    if(CMAKE_HOST_WIN32)
        qt_internal_get_prefix_script(prefix_script)
    endif()

    execute_process(
        COMMAND
            ${prefix_script}
            cmake
            --build .
            --verbose
            --parallel
        COMMAND_ECHO STDOUT
        WORKING_DIRECTORY "${QT_CUSTOM_CMAKE_BUILD_DIR}"
        RESULT_VARIABLE result
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(result)
        message(FATAL_ERROR "Failed to build cmake. Output: ${result}")
    endif()
endfunction()

# Installs cmake.
function(qt_internal_install_cmake)
    set(prefix_script "")

    if(CMAKE_HOST_WIN32)
        qt_internal_get_prefix_script(prefix_script)
    endif()

    execute_process(
        COMMAND
            ${prefix_script}
            cmake
            --install .
        COMMAND_ECHO STDOUT
        WORKING_DIRECTORY "${QT_CUSTOM_CMAKE_BUILD_DIR}"
        RESULT_VARIABLE result
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(result)
        message(FATAL_ERROR "Failed to install cmake. Output: ${result}")
    endif()
endfunction()

function(qt_internal_run_script)
    qt_internal_prepare_build_paths()
    qt_internal_clone_cmake()
    qt_internal_configure_cmake()
    qt_internal_build_cmake()
    qt_internal_install_cmake()
endfunction()

qt_internal_run_script()
