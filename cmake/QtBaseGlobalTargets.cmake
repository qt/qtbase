## QtPlatform Target:
add_library(Platform INTERFACE)
add_library(Qt::Platform ALIAS Platform)
target_include_directories(Platform
    INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/${QT_PLATFORM_DEFINITION_DIR}>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:${INSTALL_DATADIR}/${QT_PLATFORM_DEFINITION_DIR}>
    $<INSTALL_INTERFACE:${INSTALL_INCLUDEDIR}>
    )
target_compile_definitions(Platform INTERFACE ${QT_PLATFORM_DEFINITIONS})

# When building on android we need to link against the logging library
# in order to satisfy linker dependencies. Both of these libraries are part of
# the NDK.
if (ANDROID)
    target_link_libraries(Platform INTERFACE log)
endif()

set(__GlobalConfig_path_suffix "${INSTALL_CMAKE_NAMESPACE}")
qt_path_join(__GlobalConfig_build_dir ${QT_CONFIG_BUILD_DIR} ${__GlobalConfig_path_suffix})
qt_path_join(__GlobalConfig_install_dir ${QT_CONFIG_INSTALL_DIR} ${__GlobalConfig_path_suffix})

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

# Generate and install Qt6Tools config file.
configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/QtToolsConfig.cmake.in"
    "${__GlobalConfig_build_dir}/${INSTALL_CMAKE_NAMESPACE}ToolsConfig.cmake"
    INSTALL_DESTINATION "${__GlobalConfig_install_dir}"
)
write_basic_package_version_file(
    ${__GlobalConfig_build_dir}/${INSTALL_CMAKE_NAMESPACE}ToolsConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

qt_install(FILES
    "${__GlobalConfig_build_dir}/${INSTALL_CMAKE_NAMESPACE}Config.cmake"
    "${__GlobalConfig_build_dir}/${INSTALL_CMAKE_NAMESPACE}ConfigVersion.cmake"
    DESTINATION "${__GlobalConfig_install_dir}"
    COMPONENT Devel
)

qt_install(FILES
    "${__GlobalConfig_build_dir}/${INSTALL_CMAKE_NAMESPACE}ToolsConfig.cmake"
    "${__GlobalConfig_build_dir}/${INSTALL_CMAKE_NAMESPACE}ToolsConfigVersion.cmake"
    DESTINATION "${__GlobalConfig_install_dir}Tools"
    COMPONENT Devel
)

## Library to hold global features:
## These features are stored and accessed via Qt::GlobalConfig, but the
## files always lived in Qt::Core, so we keep it that way
add_library(GlobalConfig INTERFACE)
target_include_directories(GlobalConfig INTERFACE
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include/QtCore>
    $<INSTALL_INTERFACE:include>
    $<INSTALL_INTERFACE:include/QtCore>
)
qt_feature_module_begin(NO_MODULE
    PUBLIC_FILE src/corelib/global/qconfig.h
    PRIVATE_FILE src/corelib/global/qconfig_p.h
)
include("${CMAKE_CURRENT_SOURCE_DIR}/configure.cmake")
qt_feature_module_end(GlobalConfig OUT_VAR_PREFIX "__GlobalConfig_")

qt_generate_global_config_pri_file()

add_library(Qt::GlobalConfig ALIAS GlobalConfig)

add_library(GlobalConfigPrivate INTERFACE)
target_link_libraries(GlobalConfigPrivate INTERFACE GlobalConfig)
target_include_directories(GlobalConfigPrivate INTERFACE
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include/QtCore/${PROJECT_VERSION}>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include/QtCore/${PROJECT_VERSION}/QtCore>
    $<INSTALL_INTERFACE:include/QtCore/${PROJECT_VERSION}>
    $<INSTALL_INTERFACE:include/QtCore/${PROJECT_VERSION}/QtCore>
)
add_library(Qt::GlobalConfigPrivate ALIAS GlobalConfigPrivate)

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
                   cmake/QtBuild.cmake
                   cmake/QtCompilerFlags.cmake
                   cmake/QtCompilerOptimization.cmake
                   cmake/QtFeature.cmake
                   cmake/QtPlatformSupport.cmake
                   cmake/QtPlatformAndroid.cmake
                   cmake/QtPostProcess.cmake
                   cmake/QtSetup.cmake
                   cmake/QtModuleConfig.cmake.in
                   cmake/QtModuleDependencies.cmake.in
                   cmake/QtModuleToolsDependencies.cmake.in
                   cmake/QtModuleToolsConfig.cmake.in
                   cmake/QtPluginConfig.cmake.in
                   cmake/QtPluginDependencies.cmake.in
    DESTINATION "${__GlobalConfig_install_dir}"
)
if(QT_WILL_INSTALL)
    # NOTE: QtFeature.cmake is included by the Qt module config files unconditionally
    # In a prefix build, QtFeature.cmake is not copied to the build dir by default
    # Thus do it explicitly in that case so we can use the module config files in the examples
    file(COPY cmake/QtFeature.cmake DESTINATION "${__GlobalConfig_install_dir}")
endif()


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

# Configure and install the QtBuildInternals package.
set(__build_internals_path_suffix "${INSTALL_CMAKE_NAMESPACE}BuildInternals")
qt_path_join(__build_internals_build_dir ${QT_CONFIG_BUILD_DIR} ${__build_internals_path_suffix})
qt_path_join(__build_internals_install_dir ${QT_CONFIG_INSTALL_DIR}
                                           ${__build_internals_path_suffix})
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
