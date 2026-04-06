# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Compiles Java sources into a JAR using Gradle.
#
# Each module provides its own Gradle project (build.gradle, settings.gradle, etc.).
# Gradle is invoked from the source tree while the build output is redirected
# to the CMake build tree. Gradle builds an AAR and from that a JAR is extracted.
#
# The property 'isQtCMakeBuild' is always set to "true" to distinguish CMake builds
# from IDE builds where modules can depend on each other without affecting the CMake
# build which treats modules as separate and use JAR includes. It can be used as:
#   if (!findProperty('isQtCMakeBuild')) {
#       dependencies {
#           compileOnly 'org.qtproject.qt:Qt6Android'
#       }
#   }
#
# Usage:
#   qt_internal_add_jar(<target>
#       SOURCES src1.java src2.java
#       [INCLUDE_JARS jar1.jar jar2.jar]
#       [OUTPUT_DIR <dir>]                  : defaults to Qt install jar dir
#       [GRADLE_PROJECT_DIR <dir>]          : defaults to CMAKE_CURRENT_SOURCE_DIR
#   )

function(qt_internal_add_jar target)
    set(options)
    set(oneValueArgs OUTPUT_DIR GRADLE_PROJECT_DIR)
    set(multiValueArgs INCLUDE_JARS SOURCES)
    cmake_parse_arguments(arg "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Valid Gradle project files
    if(arg_GRADLE_PROJECT_DIR)
        get_filename_component(gradle_source_dir "${arg_GRADLE_PROJECT_DIR}" ABSOLUTE)
    else()
        set(gradle_source_dir "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()

    # Ease the switching phase, check for more than build.gradle because some modules
    # have it already for IDE integration, but it's not the same one for JAR builds.
    if(NOT EXISTS "${gradle_source_dir}/build.gradle"
        OR NOT EXISTS "${gradle_source_dir}/settings.gradle"
        OR NOT EXISTS "${gradle_source_dir}/gradle/libs.versions.toml")
        if(QT_FEATURE_developer_build)
            message(WARNING
                "Missing Gradle files at '${gradle_source_dir}' for ${target} build."
                "Create Gradle build files (build.gradle, settings.gradle, libs.versions.toml)"
                "or if those files exist elsewhere than the current target's source dir,"
                "set GRADLE_PROJECT_DIR to that path. Falling back to javac.")
        endif()
        _qt_internal_add_jar_with_javac("${target}" ${ARGN})
        return()
    endif()

    message(STATUS "Checking Gradle offline cache for ${target} JAR")
    _qt_internal_verify_gradle_offline_cache("${gradle_source_dir}")

    # Java source and target versions
    set(javac_target_version "${QT_ANDROID_JAVAC_TARGET}")
    if (NOT javac_target_version)
        set(javac_target_version "17")
    endif()

    set(javac_source_version "${QT_ANDROID_JAVAC_SOURCE}")
    if (NOT javac_source_version)
        set(javac_source_version "17")
    endif()

    # Android SDK versions
    set(compile_sdk "${QT_ANDROID_API_USED_FOR_JAVA}")
    string(REGEX REPLACE "^android-" "" compile_sdk "${compile_sdk}")
    set(min_sdk "${ANDROID_PLATFORM}")
    string(REGEX REPLACE "^android-" "" min_sdk "${min_sdk}")

    # Gradle build type/variant
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(build_variant "debug")
    else()
        set(build_variant "release")
    endif()

    # Evaluate common paths
    if(PROJECT_NAME STREQUAL "QtBase" OR QT_SUPERBUILD)
        set(qt_jar_dir "${QT_BUILD_DIR}/jar")
    else()
        set(qt_data_dir ${QT6_INSTALL_PREFIX}/${QT6_INSTALL_DATA})
        set(qt_jar_dir "${qt_data_dir}/jar")
    endif()

    # Output file and directory
    set(jar_output_dir "${arg_OUTPUT_DIR}")
    if(NOT jar_output_dir)
        set(jar_output_dir "${qt_jar_dir}")
    endif()
    file(TO_CMAKE_PATH "${jar_output_dir}" jar_output_dir)
    set(jar_output_file "${jar_output_dir}/${target}.jar")

    # Collect source paths
    set(absolute_sources "")
    foreach(path IN LISTS arg_SOURCES)
        get_filename_component(absolute_path "${path}" ABSOLUTE)
        list(APPEND absolute_sources "${absolute_path}")
    endforeach()
    set_property(DIRECTORY APPEND PROPERTY _qt_jar_sources "${absolute_sources}")

    # Locate the Gradle wrapper
    _qt_internal_android_gradle_template_dir(gradle_template_dir)
    _qt_internal_android_gradlew_name(gradlew_name)
    set(gradlew "${gradle_template_dir}/${gradlew_name}")

    # Build directory in the build tree to keeps source tree clean
    set(gradle_build_dir "${CMAKE_CURRENT_BINARY_DIR}/${target}_gradle")
    file(TO_CMAKE_PATH "${gradle_build_dir}" gradle_build_dir)

    # Gradle command line properties
    set(gradle_props
        "-PbuildDir=${gradle_build_dir}/build"
        "-PjarOutputDir=${jar_output_dir}"
        "-PbuildVariant=${build_variant}"
        "-PjavacSource=${javac_source_version}"
        "-PjavacTarget=${javac_target_version}"
        "-PcompileSdk=${compile_sdk}"
        "-PminSdk=${min_sdk}"
        "-PisQtCMakeBuild=true"
    )

    # Include JARs (comma-separated)
    set(include_jars_csv "")
    foreach(jar IN LISTS arg_INCLUDE_JARS)
        file(TO_CMAKE_PATH "${jar}" jar_path)
        if(include_jars_csv)
            string(APPEND include_jars_csv ",")
        endif()
        string(APPEND include_jars_csv "${jar_path}")
    endforeach()

    set(gradle_extra_args "")
    if(include_jars_csv)
        list(APPEND gradle_extra_args "-PincludeJars=${include_jars_csv}")
    endif()

    if(CMAKE_HOST_WIN32)
        # On Windows, run Gradle with --no-daemon to avoid CI cleanup issues.
        list(APPEND gradle_extra_args "--no-daemon")
    endif()

    if(NOT QT_ALLOW_DOWNLOAD)
        list(APPEND gradle_extra_args "--offline")
    endif()

    # Invoke Gradle
    add_custom_command(OUTPUT "${jar_output_file}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${jar_output_dir}"
        COMMAND "${gradlew}"
            --init-script "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/android/qt-android-init.gradle"
            --project-cache-dir "${gradle_build_dir}/.gradle"
            --no-configuration-cache
            ${gradle_extra_args}
            ${gradle_props}
            ${target}Jar
        WORKING_DIRECTORY "${gradle_source_dir}"
        DEPENDS
            "${gradle_source_dir}/build.gradle"
            "${gradle_source_dir}/settings.gradle"
            ${absolute_sources}
            ${arg_INCLUDE_JARS}
        COMMENT "Building ${target}.jar with Gradle"
        VERBATIM
    )

    add_custom_target(${target} ALL DEPENDS "${jar_output_file}")

    # Expose source files to IDEs
    foreach(path IN LISTS arg_SOURCES)
        _qt_internal_expose_source_file_to_ide(${target} "${path}")
    endforeach()

    # Target properties for install step with CMake's install_jar()
    set_target_properties(${target} PROPERTIES
        JAR_FILE "${jar_output_file}"
        INSTALL_FILES "${jar_output_file}"
    )
endfunction()

# Fallback: builds the JAR using javac via CMake's add_jar().
function(_qt_internal_add_jar_with_javac target)
    set(options)
    set(oneValueArgs OUTPUT_DIR)
    set(multiValueArgs INCLUDE_JARS SOURCES)
    cmake_parse_arguments(arg "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(javac_target_version "${QT_ANDROID_JAVAC_TARGET}")
    if (NOT javac_target_version)
        set(javac_target_version "17")
    endif()

    set(javac_source_version "${QT_ANDROID_JAVAC_SOURCE}")
    if (NOT javac_source_version)
        set(javac_source_version "17")
    endif()

    set(CMAKE_JAVA_COMPILE_FLAGS -source "${javac_source_version}" -target "${javac_target_version}"
        -Xlint:all -Xdoclint:all,-missing -classpath "${QT_ANDROID_JAR}"
    )

    set(absolute_sources "")
    foreach(path IN LISTS arg_SOURCES)
        get_filename_component(absolute_path "${path}" ABSOLUTE)
        list(APPEND absolute_sources "${absolute_path}")
    endforeach()
    set_property(DIRECTORY APPEND PROPERTY _qt_jar_sources "${absolute_sources}")

    add_jar(${target}
        SOURCES ${absolute_sources}
        INCLUDE_JARS ${arg_INCLUDE_JARS}
        OUTPUT_DIR ${arg_OUTPUT_DIR}
    )

    foreach(f IN LISTS arg_SOURCES)
        _qt_internal_expose_source_file_to_ide(${target} "${f}")
    endforeach()
endfunction()

# Verifies that a Gradle project's dependencies are cached for offline builds.
function(_qt_internal_verify_gradle_offline_cache gradle_source_dir)
    if(QT_ALLOW_DOWNLOAD)
        return()
    endif()

    set(setup_script "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/QtSetupAndroid.cmake")
    if(NOT EXISTS "${setup_script}")
        message(WARNING "Cannot find ${setup_script}, skipping the Gradle offline cache check.")
        return()
    endif()

    if(PROJECT_NAME STREQUAL "QtBase" OR QT_SUPERBUILD)
        set(qt_dir "${QtBase_SOURCE_DIR}")
    else()
        set(qt_dir "${QT6_INSTALL_PREFIX}")
    endif()

    set(action_resolve_arg "-DACTION_RESOLVE_GRADLE=ON")
    set(qt_dir_arg "-DQT_ROOT_DIR=${qt_dir}")
    set(project_dir_arg "-DGRADLE_PROJECT_DIR=${gradle_source_dir}")
    set(only_verify_arg "-DONLY_VERIFY_GRADLE_CACHE=ON")
    execute_process(
        COMMAND "${CMAKE_COMMAND}"
            "${action_resolve_arg}"
            "${qt_dir_arg}"
            "${project_dir_arg}"
            "${only_verify_arg}"
            -P "${setup_script}"
        RESULT_VARIABLE verify_result
        OUTPUT_QUIET
        ERROR_QUIET
    )

    if(NOT verify_result EQUAL 0)
        message(FATAL_ERROR
            "Downloads are disabled when building Qt and Gradle dependencies haven't been cached "
            "for such offline builds. Run the following command to populate the Gradle cache:\n"
            "  cmake ${action_resolve_arg} ${qt_dir_arg} ${project_dir_arg} -P ${setup_script}\n"
            "Or configure Qt with QT_ALLOW_DOWNLOAD=ON to allow Gradle downloads at build time.")
    endif()
endfunction()
