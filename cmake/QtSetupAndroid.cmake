# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Standalone CMake script for setting up and managing the Android development environment.
#
# Actions:
#   ACTION_RESOLVE_GRADLE:
#     Online resolve: downloads Gradle dependencies for offline Qt Android JAR builds.
#       cmake [-DQT_ROOT_DIR=<dir>] [-DGRADLE_PROJECT_DIR=<dir>] -P QtSetupAndroid.cmake
#
#     Offline verify: checks that dependencies are already cached.
#       cmake -DONLY_VERIFY_GRADLE_CACHE=ON [-DQT_ROOT_DIR=<dir>] [-DGRADLE_PROJECT_DIR=<dir>]
#             -P QtSetupAndroid.cmake
#
#     QT_ROOT_DIR: points to Qt install directory or qtbase source. Required when running
#       outside the qtbase source tree.
#
#     GRADLE_BUILD_VARIANT: Gradle build variant ('Debug' or 'Release').
#
#     QT_ANDROID_SDK_BUILD_TOOLS_REVISION: Android build tools revision.

cmake_minimum_required(VERSION 3.16)

function(_qt_setup_android_resolve_gradle_dependencies)
    if(QT_ROOT_DIR)
        get_filename_component(qt_src_dir "${QT_ROOT_DIR}/src" ABSOLUTE)
    elseif(EXISTS "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../configure")
        get_filename_component(qt_src_dir "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../src" ABSOLUTE)
    else()
        message(FATAL_ERROR
            "Cannot determine the Qt root directory. "
            "Set QT_ROOT_DIR to the Qt install root or the qtbase source.")
    endif()

    # Validate project path
    if(GRADLE_PROJECT_DIR)
        if(NOT EXISTS "${GRADLE_PROJECT_DIR}/build.gradle")
            message(FATAL_ERROR "Cannot find Gradle project at '${GRADLE_PROJECT_DIR}'.")
        endif()
        set(project_dir "${GRADLE_PROJECT_DIR}")
    elseif(EXISTS "${qt_src_dir}/android/jar/build.gradle")
        set(project_dir "${qt_src_dir}/android/jar")
    else()
        message(FATAL_ERROR "No Gradle project found. Provide one by setting GRADLE_PROJECT_DIR.")
    endif()
    file(TO_NATIVE_PATH "${project_dir}" project_dir)

    # Validate gradlew path
    set(gradlew "${qt_src_dir}/3rdparty/gradle/gradlew")
    if(CMAKE_HOST_WIN32)
        string(APPEND gradlew ".bat")
    endif()
    file(TO_NATIVE_PATH "${gradlew}" gradlew)
    if(NOT EXISTS "${gradlew}")
        message(FATAL_ERROR "Cannot find Gradle wrapper at '${gradlew}'.")
    endif()

    # Validate init script path
    set(init_script "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/android/qt-android-init.gradle")
    if(NOT EXISTS "${init_script}")
        message(FATAL_ERROR "Cannot find Gradle init script at '${init_script}'.")
    endif()

    set(extra_args "")
    set(output_args "")
    if(ONLY_VERIFY_GRADLE_CACHE)
        list(APPEND extra_args --offline)
        list(APPEND output_args OUTPUT_QUIET ERROR_QUIET)
    else()
        list(APPEND extra_args --info)
        list(APPEND output_args
            ECHO_OUTPUT_VARIABLE ECHO_ERROR_VARIABLE
            OUTPUT_VARIABLE gradle_output
            ERROR_VARIABLE gradle_error)

        message(STATUS "Resolving Gradle dependencies for Qt Android builds...")
        message(STATUS "Gradle wrapper:     ${gradlew}")
        message(STATUS "Gradle init script: ${init_script}")
        message(STATUS "JAR project:        ${project_dir}")
    endif()

    if(CMAKE_HOST_WIN32)
        set(gradlew cmd /c "${gradlew}")

        # Pass the init script relative to the Gradle working directory to avoid
        # POSIX-style misinterpretation by Gradle.
        file(RELATIVE_PATH init_script "${project_dir}" "${init_script}")

        # On Windows, run Gradle with --no-daemon to avoid CI cleanup issues.
        list(APPEND extra_args "--no-daemon")
    endif()

    set(resolve_dir "${CMAKE_CURRENT_BINARY_DIR}/qt-gradle-resolve")
    file(MAKE_DIRECTORY "${resolve_dir}")

    execute_process(
        COMMAND ${gradlew}
            --init-script "${init_script}"
            "-PbuildDir=${resolve_dir}/build"
            "-PbuildVariant=${GRADLE_BUILD_VARIANT}"
            "-PbuildToolsVersion=${QT_ANDROID_SDK_BUILD_TOOLS_REVISION}"
            --project-cache-dir "${resolve_dir}/.gradle"
            --no-configuration-cache
            "-PisQtCMakeBuild=true" # Ignore composite build relative dependencies
            ${extra_args}
            resolveDependencies
        WORKING_DIRECTORY "${project_dir}"
        RESULT_VARIABLE gradle_result
        ${output_args}
    )

    file(REMOVE_RECURSE "${resolve_dir}")

    if(NOT gradle_result EQUAL 0)
        message(FATAL_ERROR "Failed to resolve Gradle dependencies.")
    endif()
endfunction()

if(ACTION_RESOLVE_GRADLE)
    _qt_setup_android_resolve_gradle_dependencies()
endif()
