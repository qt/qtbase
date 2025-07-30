# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

set(__GlobalConfig_path_suffix "${INSTALL_CMAKE_NAMESPACE}")
qt_path_join(__GlobalConfig_build_dir ${QT_CONFIG_BUILD_DIR} ${__GlobalConfig_path_suffix})
qt_path_join(__GlobalConfig_install_dir ${QT_CONFIG_INSTALL_DIR} ${__GlobalConfig_path_suffix})
set(__GlobalConfig_install_dir_absolute "${__GlobalConfig_install_dir}")
set(__qt_bin_dir_absolute "${QT_INSTALL_DIR}/${INSTALL_BINDIR}")
set(__qt_libexec_dir_absolute "${QT_INSTALL_DIR}/${INSTALL_LIBEXECDIR}")
if(QT_WILL_INSTALL)
    # Need to prepend the install prefix when doing prefix builds, because the config install dir
    # is relative then.
    qt_path_join(__GlobalConfig_install_dir_absolute
                 ${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}
                 ${__GlobalConfig_install_dir_absolute})
    qt_path_join(__qt_bin_dir_absolute
                 ${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX} ${__qt_bin_dir_absolute})
    qt_path_join(__qt_libexec_dir_absolute
                 ${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX} ${__qt_libexec_dir_absolute})
endif()
# Compute relative path from $qt_prefix/bin dir to global CMake config install dir, to use in the
# unix-y qt-cmake shell script, to make it work even if the installed Qt is relocated.
file(RELATIVE_PATH
     __GlobalConfig_relative_path_from_bin_dir_to_cmake_config_dir
     ${__qt_bin_dir_absolute} ${__GlobalConfig_install_dir_absolute})

# Configure and install the QtBuildInternals package.
set(__build_internals_path_suffix "${INSTALL_CMAKE_NAMESPACE}BuildInternals")
qt_path_join(__build_internals_build_dir ${QT_CONFIG_BUILD_DIR} ${__build_internals_path_suffix})
qt_path_join(__build_internals_install_dir ${QT_CONFIG_INSTALL_DIR}
                                           ${__build_internals_path_suffix})
set(__build_internals_standalone_test_template_dir "QtStandaloneTestTemplateProject")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/QtBuildInternals/QtBuildInternalsConfig.cmake"
    "${__build_internals_build_dir}/${INSTALL_CMAKE_NAMESPACE}BuildInternalsConfig.cmake"
    @ONLY
    )

write_basic_package_version_file(
    "${__build_internals_build_dir}/${INSTALL_CMAKE_NAMESPACE}BuildInternalsConfigVersionImpl.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)
qt_internal_write_qt_package_version_file(
    "${INSTALL_CMAKE_NAMESPACE}BuildInternals"
    "${__build_internals_build_dir}/${INSTALL_CMAKE_NAMESPACE}BuildInternalsConfigVersion.cmake"
)

qt_install(FILES
    "${__build_internals_build_dir}/${INSTALL_CMAKE_NAMESPACE}BuildInternalsConfig.cmake"
    "${__build_internals_build_dir}/${INSTALL_CMAKE_NAMESPACE}BuildInternalsConfigVersion.cmake"
    "${__build_internals_build_dir}/${INSTALL_CMAKE_NAMESPACE}BuildInternalsConfigVersionImpl.cmake"
    "${__build_internals_build_dir}/QtBuildInternalsExtra.cmake"
    DESTINATION "${__build_internals_install_dir}"
    COMPONENT Devel
)

qt_path_join(__build_internals_standalone_test_template_path
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "cmake/QtBuildInternals/${__build_internals_standalone_test_template_dir}")

qt_copy_or_install(
    DIRECTORY "${__build_internals_standalone_test_template_path}"
    DESTINATION "${__build_internals_install_dir}")

# In prefix builds we also need to copy the files into the build dir.
if(QT_WILL_INSTALL)
    file(COPY "${__build_internals_standalone_test_template_path}"
        DESTINATION "${__build_internals_install_dir}")
endif()

set(__build_internals_extra_files
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/QtBuildInternals/QtBuildInternalsHelpers.cmake"
)

qt_copy_or_install(
    FILES ${__build_internals_extra_files}
    DESTINATION "${__build_internals_install_dir}")

# In prefix builds we also need to copy the files into the build dir.
if(QT_WILL_INSTALL)
    foreach(__build_internals_file ${__build_internals_extra_files})
        file(COPY "${__build_internals_file}" DESTINATION "${__build_internals_install_dir}")
    endforeach()
endif()

_qt_internal_append_cmake_configure_depends(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/QtBuildInternals/${__build_internals_standalone_test_template_dir}/CMakeLists.txt"
    ${__build_internals_extra_files}
)

qt_internal_create_toolchain_file()

## Library to hold global features:
## These features are stored and accessed via Qt::GlobalConfig, but the
## files always lived in Qt::Core, so we keep it that way
qt_internal_add_platform_internal_target(GlobalConfig)
target_include_directories(GlobalConfig INTERFACE
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include/QtCore>
    $<INSTALL_INTERFACE:${INSTALL_INCLUDEDIR}>
    $<INSTALL_INTERFACE:${INSTALL_INCLUDEDIR}/QtCore>
)
qt_feature_module_begin(NO_MODULE
    PUBLIC_FILE src/corelib/global/qconfig.h
    PRIVATE_FILE src/corelib/global/qconfig_p.h
)
include("${CMAKE_CURRENT_SOURCE_DIR}/configure.cmake")

# Do what mkspecs/features/uikit/default_pre.prf does, aka enable sse2 for
# simulator_and_device_builds.

qt_internal_get_first_osx_arch(__qt_osx_first_arch)
set(__qt_apple_silicon_arches "arm64;arm64e")
if(MACOS AND QT_IS_MACOS_UNIVERSAL
        AND __qt_osx_first_arch IN_LIST __qt_apple_silicon_arches)
    # The test in configure.cmake will not be run, but we know that
    # the compiler supports these intrinsics
    set(QT_FORCE_FEATURE_x86intrin ON CACHE INTERNAL "Force-enable x86 intrinsics due to platform requirements.")
    set(__QtFeature_custom_enabled_cache_variables
        TEST_x86intrin
        FEATURE_x86intrin
        QT_FEATURE_x86intrin)
endif()

if(MACOS AND QT_IS_MACOS_UNIVERSAL AND
    (__qt_osx_first_arch STREQUAL "x86_64" OR __qt_osx_first_arch STREQUAL "x86_64h"))
    set(QT_FORCE_FEATURE_neon ON CACHE INTERNAL "Force enable neon due to platform requirements.")
    set(__QtFeature_custom_enabled_cache_variables
        TEST_subarch_neon
        FEATURE_neon
        QT_FEATURE_neon)
endif()

qt_feature_module_end(GlobalConfig OUT_VAR_PREFIX "__GlobalConfig_")

# The version script support check has to happen after we determined which linker is going
# to be used. The linker decision happens in the qtbase/configure.cmake file that is processed
# above.
qt_run_linker_version_script_support()

qt_generate_global_config_pri_file()
qt_generate_global_module_pri_file()
qt_generate_global_device_pri_file()
qt_generate_qmake_and_qtpaths_wrapper_for_target()

# Depends on the global features being evaluated.
qt_internal_create_wrapper_scripts()


qt_internal_add_platform_internal_target(GlobalConfigPrivate)
target_link_libraries(GlobalConfigPrivate INTERFACE GlobalConfig)
target_include_directories(GlobalConfigPrivate INTERFACE
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include/QtCore/${PROJECT_VERSION}>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include/QtCore/${PROJECT_VERSION}/QtCore>
    $<INSTALL_INTERFACE:${INSTALL_INCLUDEDIR}/QtCore/${PROJECT_VERSION}>
    $<INSTALL_INTERFACE:${INSTALL_INCLUDEDIR}/QtCore/${PROJECT_VERSION}/QtCore>
)

qt_internal_setup_public_platform_target()

# defines PlatformCommonInternal PlatformModuleInternal PlatformPluginInternal PlatformToolInternal
# PlatformExampleInternal
include(QtInternalTargets)
qt_internal_run_common_config_tests()

# Setup sanitizer options for qtbase directory scope based on features computed above.
qt_internal_set_up_sanitizer_options()
include("${CMAKE_CURRENT_LIST_DIR}/3rdparty/extra-cmake-modules/modules/ECMEnableSanitizers.cmake")

set(__export_targets Platform
                     GlobalConfig
                     GlobalConfigPrivate
                     PlatformCommonInternal
                     PlatformModuleInternal
                     PlatformPluginInternal
                     PlatformAppInternal
                     PlatformToolInternal
                     PlatformExampleInternal
                 )
set(__export_name "${INSTALL_CMAKE_NAMESPACE}Targets")
qt_install(TARGETS ${__export_targets} EXPORT "${__export_name}")
qt_install(EXPORT ${__export_name}
           NAMESPACE ${QT_CMAKE_EXPORT_NAMESPACE}::
           DESTINATION "${__GlobalConfig_install_dir}")

qt_internal_export_modern_cmake_config_targets_file(TARGETS ${__export_targets}
    EXPORT_NAME_PREFIX ${INSTALL_CMAKE_NAMESPACE}
    CONFIG_BUILD_DIR "${__GlobalConfig_build_dir}"
    CONFIG_INSTALL_DIR "${__GlobalConfig_install_dir}"
)

# Save minimum required CMake version to use Qt.
qt_internal_get_supported_min_cmake_version_for_using_qt(supported_min_version_for_using_qt)
qt_internal_get_computed_min_cmake_version_for_using_qt(computed_min_version_for_using_qt)

# Get the lower and upper policy range to embed into the Qt6 config file.
qt_internal_get_min_new_policy_cmake_version(min_new_policy_version)
qt_internal_get_max_new_policy_cmake_version(max_new_policy_version)

# Get the list of public helper files that should be automatically included in Qt6Config.cmake.
# Used in QtConfig.cmake.in template and further down for installation purposes.
qt_internal_get_qt_build_public_helpers(__qt_cmake_public_helpers)
list(JOIN __qt_cmake_public_helpers "\n    " QT_PUBLIC_FILES_TO_INCLUDE)

# Generate and install Qt6 config file. Make sure it happens after the global feature evaluation so
# they can be accessed in the Config file if needed.
configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/QtConfig.cmake.in"
    "${__GlobalConfig_build_dir}/${INSTALL_CMAKE_NAMESPACE}Config.cmake"
    INSTALL_DESTINATION "${__GlobalConfig_install_dir}"
)

_qt_internal_export_apple_sdk_and_xcode_version_requirements(QT_CONFIG_EXTRAS_CODE)

configure_file(
    "${PROJECT_SOURCE_DIR}/cmake/QtConfigExtras.cmake.in"
    "${__GlobalConfig_build_dir}/${INSTALL_CMAKE_NAMESPACE}ConfigExtras.cmake"
    @ONLY
)

write_basic_package_version_file(
    "${__GlobalConfig_build_dir}/${INSTALL_CMAKE_NAMESPACE}ConfigVersionImpl.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)
qt_internal_write_qt_package_version_file(
    "${INSTALL_CMAKE_NAMESPACE}"
    "${__GlobalConfig_build_dir}/${INSTALL_CMAKE_NAMESPACE}ConfigVersion.cmake"
)

# Compute the reverse relative path from QtConfig.cmake to the install prefix
# this is used in QtInstallPaths to make the install paths relocatable
if(QT_WILL_INSTALL)
    get_filename_component(_clean_prefix
        "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${QT_CONFIG_INSTALL_DIR}" ABSOLUTE)
else()
    get_filename_component(_clean_prefix "${QT_CONFIG_BUILD_DIR}" ABSOLUTE)
endif()
file(RELATIVE_PATH QT_INVERSE_CONFIG_INSTALL_DIR
    "${_clean_prefix}" "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}")
configure_file(
    "${PROJECT_SOURCE_DIR}/cmake/QtInstallPaths.cmake.in"
    "${__GlobalConfig_build_dir}/QtInstallPaths.cmake"
    @ONLY
)

qt_install(FILES
    "${__GlobalConfig_build_dir}/${INSTALL_CMAKE_NAMESPACE}Config.cmake"
    "${__GlobalConfig_build_dir}/${INSTALL_CMAKE_NAMESPACE}ConfigExtras.cmake"
    "${__GlobalConfig_build_dir}/${INSTALL_CMAKE_NAMESPACE}ConfigVersion.cmake"
    "${__GlobalConfig_build_dir}/${INSTALL_CMAKE_NAMESPACE}ConfigVersionImpl.cmake"
    "${__GlobalConfig_build_dir}/QtInstallPaths.cmake"
    DESTINATION "${__GlobalConfig_install_dir}"
    COMPONENT Devel
)

qt_internal_get_qt_build_private_helpers(__qt_cmake_private_helpers)
list(TRANSFORM __qt_cmake_private_helpers PREPEND "cmake/")
list(TRANSFORM __qt_cmake_private_helpers APPEND ".cmake")

qt_internal_get_qt_build_private_files_to_install(__qt_private_files_to_install)
list(TRANSFORM __qt_private_files_to_install PREPEND "cmake/")

# Install internal CMake files.
# The functions defined inside can not be used in public projects.
# They can only be used while building Qt itself.
set(__private_files
    ${__qt_cmake_private_helpers}
    ${__qt_private_files_to_install}
)
qt_copy_or_install(FILES
                   ${__private_files}
    DESTINATION "${__GlobalConfig_install_dir}"
)

# Install our custom platform modules.
qt_copy_or_install(DIRECTORY cmake/platforms
    DESTINATION "${__GlobalConfig_install_dir}"
)

# Install public config.tests files.
qt_copy_or_install(DIRECTORY
    "config.tests/static_link_order"
    "config.tests/binary_for_strip"
    DESTINATION "${__GlobalConfig_install_dir}/config.tests"
)

# Install qt-internal-strip and qt-internal-ninja files.
set(__qt_internal_strip_wrapper_programs
    libexec/qt-internal-strip.in
    libexec/qt-internal-ninja.in
)
set(__qt_internal_strip_wrapper_files
    libexec/qt-internal-strip.bat.in
    libexec/qt-internal-ninja.bat.in
)
set(__qt_internal_strip_wrappers
    ${__qt_internal_strip_wrapper_programs}
    ${__qt_internal_strip_wrapper_files}
)
qt_copy_or_install(PROGRAMS
    ${__qt_internal_strip_wrapper_programs}
    DESTINATION "${__GlobalConfig_install_dir}/libexec"
)
qt_copy_or_install(FILES
    ${__qt_internal_strip_wrapper_files}
    DESTINATION "${__GlobalConfig_install_dir}/libexec"
)
if(QT_WILL_INSTALL)
    foreach(__qt_internal_strip_wrapper ${__qt_internal_strip_wrappers})
        file(COPY "${__qt_internal_strip_wrapper}"
             DESTINATION "${__GlobalConfig_build_dir}/libexec")
    endforeach()
endif()

# Wrap previously queried helpers file.
list(TRANSFORM __qt_cmake_public_helpers PREPEND "cmake/")
list(TRANSFORM __qt_cmake_public_helpers APPEND ".cmake")

qt_internal_get_qt_build_public_files_to_install(__qt_public_files_to_install)
list(TRANSFORM __qt_public_files_to_install PREPEND "cmake/")

# Install public CMake files.
# The functions defined inside can be used in both public projects and while building Qt.
# Usually we put such functions into Qt6CoreMacros.cmake, but that's getting bloated.
# These files will be included by Qt6Config.cmake.
set(__public_files
    ${__qt_cmake_public_helpers}
    ${__qt_public_files_to_install}
)

qt_copy_or_install(FILES ${__public_files} DESTINATION "${__GlobalConfig_install_dir}")

# In prefix builds we also need to copy the files into the build config directory, so that the
# build-dir Qt6Config.cmake finds the files when building examples in-tree.
if(QT_WILL_INSTALL)
    foreach(_public_file ${__public_files})
        file(COPY "${_public_file}" DESTINATION "${__GlobalConfig_build_dir}")
    endforeach()
endif()

qt_copy_or_install(DIRECTORY "cmake/3rdparty" DESTINATION "${__GlobalConfig_install_dir}")

# In prefix builds we also need to copy the files into the build config directory, so that the
# build-dir Qt6Config.cmake finds the files when building other repos in a top-level build.
if(QT_WILL_INSTALL)
    file(COPY "cmake/3rdparty" DESTINATION "${__GlobalConfig_build_dir}")
endif()

# Install our custom Find modules, which will be used by the find_dependency() calls
# inside the generated ModuleDependencies cmake files.
qt_copy_or_install(DIRECTORY cmake/
    DESTINATION "${__GlobalConfig_install_dir}"
    FILES_MATCHING PATTERN "Find*.cmake"
    PATTERN "tests" EXCLUDE
    PATTERN "3rdparty" EXCLUDE
    PATTERN "macos" EXCLUDE
    PATTERN "ios" EXCLUDE
    PATTERN "visionos" EXCLUDE
    PATTERN "platforms" EXCLUDE
    PATTERN "QtBuildInternals" EXCLUDE
)

# In prefix builds we also need to copy the files into the build config directory, so that the
# build-dir Qt6Config.cmake finds the files when building examples as ExternalProjects.
if(QT_WILL_INSTALL)
    file(COPY cmake/
        DESTINATION "${__GlobalConfig_build_dir}"
        FILES_MATCHING PATTERN "Find*.cmake"
        PATTERN "tests" EXCLUDE
        PATTERN "3rdparty" EXCLUDE
        PATTERN "macos" EXCLUDE
        PATTERN "ios" EXCLUDE
        PATTERN "visionos" EXCLUDE
        PATTERN "platforms" EXCLUDE
        PATTERN "QtBuildInternals" EXCLUDE
    )
endif()

if(APPLE)
    if(MACOS)
        set(platform_shortname "macos")
    elseif(IOS)
        set(platform_shortname "ios")
    elseif(VISIONOS)
        set(platform_shortname "visionos")
    endif()

    # Info.plist
    qt_copy_or_install(FILES "cmake/${platform_shortname}/Info.plist.app.in"
        DESTINATION "${__GlobalConfig_install_dir}/${platform_shortname}"
    )
    # For examples built as part of prefix build before install
    file(COPY "cmake/${platform_shortname}/Info.plist.app.in"
        DESTINATION "${__GlobalConfig_build_dir}/${platform_shortname}"
    )

    # Privacy manifest
    qt_copy_or_install(FILES "cmake/${platform_shortname}/PrivacyInfo.xcprivacy"
        DESTINATION "${__GlobalConfig_install_dir}/${platform_shortname}"
    )
    # For examples built as part of prefix build before install
    file(COPY "cmake/${platform_shortname}/PrivacyInfo.xcprivacy"
        DESTINATION "${__GlobalConfig_build_dir}/${platform_shortname}"
    )

    if(MACOS)
        # Test entitlements
        qt_copy_or_install(FILES "cmake/${platform_shortname}/test.entitlements.plist"
            DESTINATION "${__GlobalConfig_install_dir}/${platform_shortname}"
        )
        file(COPY "cmake/${platform_shortname}/test.entitlements.plist"
            DESTINATION "${__GlobalConfig_build_dir}/${platform_shortname}"
        )
    elseif(IOS)
        qt_copy_or_install(FILES "cmake/ios/LaunchScreen.storyboard"
            DESTINATION "${__GlobalConfig_install_dir}/ios"
        )
        # For examples built as part of prefix build before install
        file(COPY "cmake/ios/LaunchScreen.storyboard"
            DESTINATION "${__GlobalConfig_build_dir}/ios"
        )
    endif()
elseif(WASM)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/util/wasm/wasmtestrunner/qt-wasmtestrunner.py"
        "${QT_BUILD_DIR}/${INSTALL_LIBEXECDIR}/qt-wasmtestrunner.py" @ONLY)
    qt_install(PROGRAMS "${QT_BUILD_DIR}/${INSTALL_LIBEXECDIR}/qt-wasmtestrunner.py"
        DESTINATION "${INSTALL_LIBEXECDIR}")

    if(QT_FEATURE_shared)
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/util/wasm/preload/preload_qml_imports.py"
            "${QT_BUILD_DIR}/${INSTALL_LIBEXECDIR}/preload_qml_imports.py" COPYONLY)
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/util/wasm/preload/preload_qt_plugins.py"
            "${QT_BUILD_DIR}/${INSTALL_LIBEXECDIR}/preload_qt_plugins.py" COPYONLY)
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/util/wasm/preload/generate_default_preloads.sh.in"
            "${QT_BUILD_DIR}/${INSTALL_LIBEXECDIR}/generate_default_preloads.sh.in" COPYONLY)
        qt_install(PROGRAMS "${QT_BUILD_DIR}/${INSTALL_LIBEXECDIR}/preload_qml_imports.py"
            DESTINATION "${INSTALL_LIBEXECDIR}")
        qt_install(PROGRAMS "${QT_BUILD_DIR}/${INSTALL_LIBEXECDIR}/preload_qt_plugins.py"
            DESTINATION "${INSTALL_LIBEXECDIR}")
        qt_install(PROGRAMS "${QT_BUILD_DIR}/${INSTALL_LIBEXECDIR}/generate_default_preloads.sh.in"
            DESTINATION "${INSTALL_LIBEXECDIR}")
    endif()
endif()

# Install CI support files to libexec.
qt_path_join(__qt_libexec_install_dir "${QT_INSTALL_DIR}" "${INSTALL_LIBEXECDIR}")
qt_copy_or_install(FILES coin/instructions/qmake/ensure_pro_file.cmake
    DESTINATION "${__qt_libexec_install_dir}")
qt_copy_or_install(PROGRAMS "util/testrunner/qt-testrunner.py"
    DESTINATION "${__qt_libexec_install_dir}")
qt_copy_or_install(PROGRAMS "util/testrunner/sanitizer-testrunner.py"
    DESTINATION "${__qt_libexec_install_dir}")
