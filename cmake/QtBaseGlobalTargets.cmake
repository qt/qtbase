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

include(QtToolchainHelpers)
qt_internal_create_toolchain_file()

include(QtWrapperScriptHelpers)
qt_internal_create_wrapper_scripts()

## Library to hold global features:
## These features are stored and accessed via Qt::GlobalConfig, but the
## files always lived in Qt::Core, so we keep it that way
add_library(GlobalConfig INTERFACE)
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
if(UIKIT AND NOT QT_UIKIT_SDK)
    set(__QtFeature_custom_enabled_cache_variables
        TEST_subarch_sse2
        FEATURE_sse2
        QT_FEATURE_sse2)
endif()

qt_feature_module_end(GlobalConfig OUT_VAR_PREFIX "__GlobalConfig_")

qt_generate_global_config_pri_file()
qt_generate_global_module_pri_file()
qt_generate_global_device_pri_file()
qt_generate_qmake_wrapper_for_target()

add_library(Qt::GlobalConfig ALIAS GlobalConfig)

add_library(GlobalConfigPrivate INTERFACE)
target_link_libraries(GlobalConfigPrivate INTERFACE GlobalConfig)
target_include_directories(GlobalConfigPrivate INTERFACE
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include/QtCore/${PROJECT_VERSION}>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include/QtCore/${PROJECT_VERSION}/QtCore>
    $<INSTALL_INTERFACE:${INSTALL_INCLUDEDIR}/QtCore/${PROJECT_VERSION}>
    $<INSTALL_INTERFACE:${INSTALL_INCLUDEDIR}/QtCore/${PROJECT_VERSION}/QtCore>
)
add_library(Qt::GlobalConfigPrivate ALIAS GlobalConfigPrivate)

include(QtPublicTargetsHelpers)
qt_internal_setup_public_platform_target()

# defines PlatformCommonInternal PlatformModuleInternal PlatformPluginInternal PlatformToolInternal
include(QtInternalTargets)

set(__export_targets Platform
                     GlobalConfig
                     GlobalConfigPrivate
                     PlatformCommonInternal
                     PlatformModuleInternal
                     PlatformPluginInternal
                     PlatformAppInternal
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
                   cmake/Qt3rdPartyLibraryHelpers.cmake
                   cmake/QtAppHelpers.cmake
                   cmake/QtAutogenHelpers.cmake
                   cmake/QtBuild.cmake
                   cmake/QtBuildInformation.cmake
                   cmake/QtCMakeHelpers.cmake
                   cmake/QtCompatibilityHelpers.cmake
                   cmake/QtCompilerFlags.cmake
                   cmake/QtCompilerOptimization.cmake
                   cmake/QtConfigDependencies.cmake.in
                   cmake/QtDbusHelpers.cmake
                   cmake/QtDocsHelpers.cmake
                   cmake/QtExecutableHelpers.cmake
                   cmake/QtFeature.cmake
                   cmake/QtFeatureCommon.cmake
                   cmake/QtFileConfigure.txt.in
                   cmake/QtFindPackageHelpers.cmake
                   cmake/QtFindWrapConfigExtra.cmake.in
                   cmake/QtFindWrapHelper.cmake
                   cmake/QtFinishPrlFile.cmake
                   cmake/QtFlagHandlingHelpers.cmake
                   cmake/QtFrameworkHelpers.cmake
                   cmake/QtGenerateExtPri.cmake
                   cmake/QtGenerateLibHelpers.cmake
                   cmake/QtGenerateLibPri.cmake
                   cmake/QtGlobalStateHelpers.cmake
                   cmake/QtHeadersClean.cmake
                   cmake/QtInstallHelpers.cmake
                   cmake/QtJavaHelpers.cmake
                   cmake/QtLalrHelpers.cmake
                   cmake/QtModuleConfig.cmake.in
                   cmake/QtModuleDependencies.cmake.in
                   cmake/QtModuleHelpers.cmake
                   cmake/QtModuleToolsConfig.cmake.in
                   cmake/QtModuleToolsDependencies.cmake.in
                   cmake/QtModuleToolsVersionlessTargets.cmake.in
                   cmake/QtNoLinkTargetHelpers.cmake
                   cmake/QtPlatformAndroid.cmake
                   cmake/QtPlatformSupport.cmake
                   cmake/QtPluginConfig.cmake.in
                   cmake/QtPluginDependencies.cmake.in
                   cmake/QtPluginHelpers.cmake
                   cmake/QtPlugins.cmake.in
                   cmake/QtPostProcess.cmake
                   cmake/QtPostProcessHelpers.cmake
                   cmake/QtPrecompiledHeadersHelpers.cmake
                   cmake/QtPriHelpers.cmake
                   cmake/QtPrlHelpers.cmake
                   cmake/QtPublicTargetsHelpers.cmake
                   cmake/QtProcessConfigureArgs.cmake
                   cmake/QtQmakeHelpers.cmake
                   cmake/QtResourceHelpers.cmake
                   cmake/QtRpathHelpers.cmake
                   cmake/QtSanitizerHelpers.cmake
                   cmake/QtScopeFinalizerHelpers.cmake
                   cmake/QtSeparateDebugInfo.Info.plist.in
                   cmake/QtSeparateDebugInfo.cmake
                   cmake/QtSetup.cmake
                   cmake/QtSimdHelpers.cmake
                   cmake/QtStartupHelpers.cmake
                   cmake/QtStandaloneTestsConfig.cmake.in
                   cmake/QtSyncQtHelpers.cmake
                   cmake/QtTargetHelpers.cmake
                   cmake/QtTestHelpers.cmake
                   cmake/QtToolchainHelpers.cmake
                   cmake/QtToolHelpers.cmake
                   cmake/QtWrapperScriptHelpers.cmake
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
