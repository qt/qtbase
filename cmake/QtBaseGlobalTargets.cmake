## QtPlatform Target:
add_library(Platform INTERFACE)
add_library(Qt::Platform ALIAS Platform)
target_include_directories(Platform
    INTERFACE
    $<BUILD_INTERFACE:${QT_PLATFORM_DEFINITION_DIR_ABSOLUTE}>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/${INSTALL_INCLUDEDIR}>
    $<INSTALL_INTERFACE:${QT_PLATFORM_DEFINITION_DIR}>
    $<INSTALL_INTERFACE:${INSTALL_INCLUDEDIR}>
    )
target_compile_definitions(Platform INTERFACE ${QT_PLATFORM_DEFINITIONS})

# When building on android we need to link against the logging library
# in order to satisfy linker dependencies. Both of these libraries are part of
# the NDK.
if (ANDROID)
    target_link_libraries(Platform INTERFACE log)
endif()

qt_enable_msvc_cplusplus_define(Platform INTERFACE)

set(__GlobalConfig_path_suffix "${INSTALL_CMAKE_NAMESPACE}")
qt_path_join(__GlobalConfig_build_dir ${QT_CONFIG_BUILD_DIR} ${__GlobalConfig_path_suffix})
qt_path_join(__GlobalConfig_install_dir ${QT_CONFIG_INSTALL_DIR} ${__GlobalConfig_path_suffix})
set(__GlobalConfig_install_dir_absolute "${__GlobalConfig_install_dir}")
set(__qt_bin_dir_absolute "${QT_INSTALL_DIR}/${INSTALL_BINDIR}")
if(QT_WILL_INSTALL)
    # Need to prepend the install prefix when doing prefix builds, because the config install dir
    # is relative then.
    qt_path_join(__GlobalConfig_install_dir_absolute
                 ${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}
                 ${__GlobalConfig_install_dir_absolute})
    qt_path_join(__qt_bin_dir_absolute
                 ${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX} ${__qt_bin_dir_absolute})
endif()
# Compute relative path from $qt_prefix/bin dir to global CMake config install dir, to use in the
# unix-y qt-cmake shell script, to make it work even if the installed Qt is relocated.
file(RELATIVE_PATH
     __GlobalConfig_relative_path_from_bin_dir_to_cmake_config_dir
     ${__qt_bin_dir_absolute} ${__GlobalConfig_install_dir_absolute})

# Generate and install Qt6 config file.
configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/QtConfig.cmake.in"
    "${__GlobalConfig_build_dir}/${INSTALL_CMAKE_NAMESPACE}Config.cmake"
    INSTALL_DESTINATION "${__GlobalConfig_install_dir}"
)

write_basic_package_version_file(
    ${__GlobalConfig_build_dir}/${INSTALL_CMAKE_NAMESPACE}ConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

qt_install(FILES
    "${__GlobalConfig_build_dir}/${INSTALL_CMAKE_NAMESPACE}Config.cmake"
    "${__GlobalConfig_build_dir}/${INSTALL_CMAKE_NAMESPACE}ConfigVersion.cmake"
    DESTINATION "${__GlobalConfig_install_dir}"
    COMPONENT Devel
)

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

qt_install(FILES
    "${__build_internals_build_dir}/${INSTALL_CMAKE_NAMESPACE}BuildInternalsConfig.cmake"
    "${__build_internals_build_dir}/QtBuildInternalsExtra.cmake"
    DESTINATION "${__build_internals_install_dir}"
    COMPONENT Devel
)
qt_copy_or_install(
    FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/QtBuildInternals/QtBuildInternalsAndroid.cmake"
    DESTINATION "${__build_internals_install_dir}")
qt_copy_or_install(
    DIRECTORY
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/QtBuildInternals/${__build_internals_standalone_test_template_dir}"
    DESTINATION "${__build_internals_install_dir}")

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/QtBuildInternals/${__build_internals_standalone_test_template_dir}/CMakeLists.txt")

# Generate toolchain file for convenience
if(QT_HOST_PATH)
    get_filename_component(init_qt_host_path "${QT_HOST_PATH}" ABSOLUTE)
    set(init_qt_host_path "set(QT_HOST_PATH \"${init_qt_host_path}\" CACHE PATH \"\" FORCE)")
endif()

if(CMAKE_TOOLCHAIN_FILE)
    set(init_original_toolchain_file "set(__qt_chainload_toolchain_file \"${CMAKE_TOOLCHAIN_FILE}\")")
endif()

if(VCPKG_CHAINLOAD_TOOLCHAIN_FILE)
    list(APPEND init_vcpkg "set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE \"${VCPKG_CHAINLOAD_TOOLCHAIN_FILE}\")")
endif()

if(VCPKG_TARGET_TRIPLET)
    list(APPEND init_vcpkg "set(VCPKG_TARGET_TRIPLET \"${VCPKG_TARGET_TRIPLET}\" CACHE STRING \"\")")
endif()

# On Windows compilers aren't easily mixed. Avoid that qtbase is built using cl.exe for example and then for another
# build gcc is picked up from %PATH%. The same goes when using a custom compiler on other platforms, such as ICC.
list(APPEND init_platform "set(CMAKE_CXX_COMPILER \"${CMAKE_CXX_COMPILER}\" CACHE STRING \"\")")
list(APPEND init_platform "set(CMAKE_C_COMPILER \"${CMAKE_C_COMPILER}\" CACHE STRING \"\")")

if(APPLE)
    # For simulator_and_device build, we should not explicitly set the sysroot.
    list(LENGTH CMAKE_OSX_ARCHITECTURES _qt_osx_architectures_count)
    if(CMAKE_OSX_SYSROOT AND NOT _qt_osx_architectures_count GREATER 1 AND UIKIT)
        list(APPEND init_platform "set(CMAKE_OSX_SYSROOT \"${CMAKE_OSX_SYSROOT}\" CACHE PATH \"\")")
    endif()
    unset(_qt_osx_architectures_count)

    if(CMAKE_OSX_DEPLOYMENT_TARGET)
        list(APPEND init_platform
            "set(CMAKE_OSX_DEPLOYMENT_TARGET \"${CMAKE_OSX_DEPLOYMENT_TARGET}\" CACHE STRING \"\")")
    endif()

    if(UIKIT)
        list(APPEND init_platform
            "set(CMAKE_SYSTEM_NAME \"${CMAKE_SYSTEM_NAME}\" CACHE STRING \"\")")
        set(_qt_osx_architectures_escaped "${CMAKE_OSX_ARCHITECTURES}")
        string(REPLACE ";" "LITERAL_SEMICOLON"
            _qt_osx_architectures_escaped "${_qt_osx_architectures_escaped}")
        list(APPEND init_platform
            "set(CMAKE_OSX_ARCHITECTURES \"${_qt_osx_architectures_escaped}\" CACHE STRING \"\")")
        unset(_qt_osx_architectures_escaped)
    endif()
elseif(ANDROID)
    list(APPEND init_platform "set(ANDROID_NATIVE_API_LEVEL \"${ANDROID_NATIVE_API_LEVEL}\" CACHE STRING \"\")")
    list(APPEND init_platform "set(ANDROID_STL \"${ANDROID_STL}\" CACHE STRING \"\")")
    list(APPEND init_platform "set(ANDROID_ABI \"${ANDROID_ABI}\" CACHE STRING \"\")")
    list(APPEND init_platform "if (NOT DEFINED ANDROID_SDK_ROOT)")
    list(APPEND init_platform "    set(ANDROID_SDK_ROOT \"${ANDROID_SDK_ROOT}\" CACHE STRING \"\")")
    list(APPEND init_platform "endif()")
endif()

string(REPLACE ";" "\n" init_vcpkg "${init_vcpkg}")
string(REPLACE ";" "\n" init_platform "${init_platform}")
string(REPLACE "LITERAL_SEMICOLON" ";" init_platform "${init_platform}")
qt_compute_relative_path_from_cmake_config_dir_to_prefix()
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/qt.toolchain.cmake.in" "${__GlobalConfig_build_dir}/qt.toolchain.cmake" @ONLY)
unset(qt_path_from_cmake_config_dir_to_prefix)
qt_install(FILES "${__GlobalConfig_build_dir}/qt.toolchain.cmake" DESTINATION "${__GlobalConfig_install_dir}" COMPONENT Devel)

# Also provide a convenience cmake wrapper
if(UNIX)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/bin/qt-cmake.in" "${QT_BUILD_DIR}/${INSTALL_BINDIR}/qt-cmake" @ONLY)
    qt_install(PROGRAMS "${QT_BUILD_DIR}/${INSTALL_BINDIR}/qt-cmake" DESTINATION "${INSTALL_BINDIR}")
else()
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/bin/qt-cmake.bat.in" "${QT_BUILD_DIR}/${INSTALL_BINDIR}/qt-cmake.bat" @ONLY)
    qt_install(PROGRAMS "${QT_BUILD_DIR}/${INSTALL_BINDIR}/qt-cmake.bat" DESTINATION "${INSTALL_BINDIR}")
endif()

# Provide a private convenience wrapper with options which should not be propagated via the public
# qt-cmake wrapper e.g. CMAKE_GENERATOR.
# These options can not be set in a toolchain file, but only on the command line.
# These options should not be in the public wrapper, because a consumer of Qt might want to build
# their CMake app with the Unix Makefiles generator, while Qt should be built with the Ninja
# generator.
# The private wrapper is more conveient for building Qt itself, because a developer doesn't need
# to specify the same options for each qt module built.
set(__qt_cmake_extra "-G\"${CMAKE_GENERATOR}\"")
if(UNIX)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/bin/qt-cmake.in"
        "${QT_BUILD_DIR}/${INSTALL_BINDIR}/qt-cmake-private" @ONLY)
qt_install(PROGRAMS "${QT_BUILD_DIR}/${INSTALL_BINDIR}/qt-cmake-private" DESTINATION "${INSTALL_BINDIR}")
else()
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/bin/qt-cmake.bat.in"
        "${QT_BUILD_DIR}/${INSTALL_BINDIR}/qt-cmake-private.bat" @ONLY)
qt_install(PROGRAMS "${QT_BUILD_DIR}/${INSTALL_BINDIR}/qt-cmake-private.bat" DESTINATION "${INSTALL_BINDIR}")
endif()
unset(__qt_cmake_extra)

# Provide a private convenience wrapper to configure and build one or more standalone tests.
# Calling CMake directly on a Qt test project won't work because the project does not call
# find_package(Qt...) to get all dependencies like examples do.
# Instead a template CMakeLists.txt project is used which sets up all the necessary private bits
# and then calls add_subdirectory on the provided project path.
set(__qt_cmake_standalone_test_bin_name "qt-cmake-standalone-test")
set(__qt_cmake_private_path
    "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${INSTALL_BINDIR}/qt-cmake-private")
set(__qt_cmake_standalone_test_path
    "${__build_internals_install_dir}/${__build_internals_standalone_test_template_dir}")

if(QT_WILL_INSTALL)
    # Need to prepend the install prefix when doing prefix builds, because the build internals
    # install dir is relative in that case..
    qt_path_join(__qt_cmake_standalone_test_path
                 "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}"
                 "${__qt_cmake_standalone_test_path}")
endif()
if(UNIX)
    string(PREPEND __qt_cmake_private_path "exec ")
    set(__qt_cmake_standalone_passed_args "\"$@\" -DPWD=\"$PWD\"")
else()
    string(APPEND __qt_cmake_standalone_test_bin_name ".bat")
    string(APPEND __qt_cmake_private_path ".bat")
    set(__qt_cmake_standalone_passed_args "%* -DPWD=\"%CD%\"")
endif()
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/bin/qt-cmake-standalone-test.in"
               "${QT_BUILD_DIR}/${INSTALL_BINDIR}/${__qt_cmake_standalone_test_bin_name}")
qt_install(PROGRAMS "${QT_BUILD_DIR}/${INSTALL_BINDIR}/${__qt_cmake_standalone_test_bin_name}"
           DESTINATION "${INSTALL_BINDIR}")

# Create an installation script that the CI can use to handle installation for both
# single and multiple configurations.
set(__qt_cmake_install_script_name "qt-cmake-private-install.cmake")
if(CMAKE_CONFIGURATION_TYPES)
    set(__qt_configured_configs "${CMAKE_CONFIGURATION_TYPES}")
elseif(CMAKE_BUILD_TYPE)
    set(__qt_configured_configs "${CMAKE_BUILD_TYPE}")
endif()
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/bin/${__qt_cmake_install_script_name}.in"
    "${QT_BUILD_DIR}/${INSTALL_BINDIR}/${__qt_cmake_install_script_name}" @ONLY)
qt_install(PROGRAMS "${QT_BUILD_DIR}/${INSTALL_BINDIR}/${__qt_cmake_install_script_name}"
           DESTINATION "${INSTALL_BINDIR}")

## Library to hold global features:
## These features are stored and accessed via Qt::GlobalConfig, but the
## files always lived in Qt::Core, so we keep it that way
add_library(GlobalConfig INTERFACE)
target_include_directories(GlobalConfig INTERFACE
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/${INSTALL_INCLUDEDIR}>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/${INSTALL_INCLUDEDIR}/QtCore>
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
if(UIKIT AND NOT QT_UIKIT_SDK)
    set(__QtFeature_custom_enabled_cache_variables
        TEST_subarch_sse2
        FEATURE_sse2
        QT_FEATURE_sse2)
endif()

qt_feature_module_end(GlobalConfig OUT_VAR_PREFIX "__GlobalConfig_")

qt_generate_global_config_pri_file()
qt_generate_global_module_pri_file()
qt_generate_qt_conf()

add_library(Qt::GlobalConfig ALIAS GlobalConfig)

add_library(GlobalConfigPrivate INTERFACE)
target_link_libraries(GlobalConfigPrivate INTERFACE GlobalConfig)
target_include_directories(GlobalConfigPrivate INTERFACE
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/${INSTALL_INCLUDEDIR}/QtCore/${PROJECT_VERSION}>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/${INSTALL_INCLUDEDIR}/QtCore/${PROJECT_VERSION}/QtCore>
    $<INSTALL_INTERFACE:${INSTALL_INCLUDEDIR}/QtCore/${PROJECT_VERSION}>
    $<INSTALL_INTERFACE:${INSTALL_INCLUDEDIR}/QtCore/${PROJECT_VERSION}/QtCore>
)
add_library(Qt::GlobalConfigPrivate ALIAS GlobalConfigPrivate)

# Propagate minimum C++ 17 via Platform to Qt consumers (apps), after the global features
# are computed.
qt_set_language_standards_interface_compile_features(Platform)

# By default enable utf8 sources for both Qt and Qt consumers. Can be opted out.
qt_enable_utf8_sources(Platform)

# defines PlatformCommonInternal PlatformModuleInternal PlatformPluginInternal PlatformToolInternal
include(QtInternalTargets)

set(__export_targets Platform
                     GlobalConfig
                     GlobalConfigPrivate
                     PlatformCommonInternal
                     PlatformModuleInternal
                     PlatformPluginInternal
                     PlatformToolInternal)
set(__export_name "${INSTALL_CMAKE_NAMESPACE}Targets")
qt_install(TARGETS ${__export_targets} EXPORT "${__export_name}")
qt_install(EXPORT ${__export_name}
           NAMESPACE ${QT_CMAKE_EXPORT_NAMESPACE}::
           DESTINATION "${__GlobalConfig_install_dir}")

qt_internal_export_modern_cmake_config_targets_file(TARGETS ${__export_targets}
                                                    EXPORT_NAME_PREFIX ${INSTALL_CMAKE_NAMESPACE}
                                                    CONFIG_INSTALL_DIR
                                                    ${__GlobalConfig_install_dir})

## Install some QtBase specific CMake files:
qt_copy_or_install(FILES
                   cmake/ModuleDescription.json.in
                   cmake/Qt3rdPartyLibraryConfig.cmake.in
                   cmake/QtBuild.cmake
                   cmake/QtBuildInformation.cmake
                   cmake/QtCompilerFlags.cmake
                   cmake/QtCompilerOptimization.cmake
                   cmake/QtFeature.cmake
                   cmake/QtFinishPrlFile.cmake
                   cmake/QtFindWrapHelper.cmake
                   cmake/QtFindWrapConfigExtra.cmake.in
                   cmake/QtFileConfigure.txt.in
                   cmake/QtGenerateExtPri.cmake
                   cmake/QtGenerateLibPri.cmake
                   cmake/QtPlatformSupport.cmake
                   cmake/QtPlatformAndroid.cmake
                   cmake/QtPostProcess.cmake
                   cmake/QtSeparateDebugInfo.Info.plist.in
                   cmake/QtSeparateDebugInfo.cmake
                   cmake/QtSetup.cmake
                   cmake/QtModuleConfig.cmake.in
                   cmake/QtModuleDependencies.cmake.in
                   cmake/QtModuleToolsDependencies.cmake.in
                   cmake/QtModuleToolsConfig.cmake.in
                   cmake/QtModuleToolsVersionlessTargets.cmake.in
                   cmake/QtStandaloneTestsConfig.cmake.in
                   cmake/QtPlugins.cmake.in
                   cmake/QtPluginConfig.cmake.in
                   cmake/QtPluginDependencies.cmake.in
    DESTINATION "${__GlobalConfig_install_dir}"
)

file(COPY cmake/QtFeature.cmake DESTINATION "${__GlobalConfig_build_dir}")

# TODO: Check whether this is the right place to install these
qt_copy_or_install(DIRECTORY cmake/3rdparty DESTINATION "${__GlobalConfig_install_dir}")

# Install our custom Find modules, which will be used by the find_dependency() calls
# inside the generated ModuleDependencies cmake files.
qt_copy_or_install(DIRECTORY cmake/
    DESTINATION "${__GlobalConfig_install_dir}"
    FILES_MATCHING PATTERN "Find*.cmake"
    PATTERN "tests" EXCLUDE
    PATTERN "3rdparty" EXCLUDE
)

if(MACOS)
    qt_copy_or_install(FILES
        cmake/macos/MacOSXBundleInfo.plist.in
        DESTINATION "${__GlobalConfig_install_dir}/macos"
    )
endif()
