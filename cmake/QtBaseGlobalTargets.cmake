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
set(config_install_dir "${INSTALL_LIBDIR}/cmake/${INSTALL_CMAKE_NAMESPACE}")

# Generate and install Qt5 config file.
configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/QtConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${INSTALL_CMAKE_NAMESPACE}Config.cmake"
    INSTALL_DESTINATION "${config_install_dir}"
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${INSTALL_CMAKE_NAMESPACE}ConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${INSTALL_CMAKE_NAMESPACE}Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${INSTALL_CMAKE_NAMESPACE}ConfigVersion.cmake"
    DESTINATION "${config_install_dir}"
    COMPONENT Devel
)

# Generate and install Qt5Tools config file.
configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/QtToolsConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${INSTALL_CMAKE_NAMESPACE}ToolsConfig.cmake"
    INSTALL_DESTINATION "${config_install_dir}"
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${INSTALL_CMAKE_NAMESPACE}ToolsConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${INSTALL_CMAKE_NAMESPACE}ToolsConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${INSTALL_CMAKE_NAMESPACE}ToolsConfigVersion.cmake"
    DESTINATION "${config_install_dir}Tools"
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
qt_feature_module_begin(LIBRARY Core
    PUBLIC_FILE src/corelib/global/qconfig.h
    PRIVATE_FILE src/corelib/global/qconfig_p.h
)
include("${CMAKE_CURRENT_SOURCE_DIR}/configure.cmake")
qt_feature_module_end(GlobalConfig)

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

install(TARGETS Platform GlobalConfig GlobalConfigPrivate EXPORT "${INSTALL_CMAKE_NAMESPACE}Targets")
install(EXPORT "${INSTALL_CMAKE_NAMESPACE}Targets" NAMESPACE ${QT_CMAKE_EXPORT_NAMESPACE}:: DESTINATION "${config_install_dir}")
export(EXPORT "${INSTALL_CMAKE_NAMESPACE}Targets")

qt_internal_export_modern_cmake_config_targets_file(TARGETS Platform GlobalConfig GlobalConfigPrivate
                                                    EXPORT_NAME_PREFIX ${INSTALL_CMAKE_NAMESPACE}
                                                    CONFIG_INSTALL_DIR ${config_install_dir})

## Install some QtBase specific CMake files:
install(FILES
        cmake/QtBuild.cmake
        cmake/QtCompilerFlags.cmake
        cmake/QtCompilerOptimization.cmake
        cmake/QtFeature.cmake
        cmake/QtPlatformSupport.cmake
        cmake/QtPostProcess.cmake
        cmake/QtSetup.cmake
        cmake/QtModuleConfig.cmake.in
        cmake/QtModuleDependencies.cmake.in
        cmake/QtModuleToolsDependencies.cmake.in
        cmake/QtModuleToolsConfig.cmake.in
    DESTINATION "${config_install_dir}"
)
# TODO: Check whether this is the right place to install these
install(DIRECTORY cmake/3rdparty
    DESTINATION "${config_install_dir}"
)

# Install our custom Find modules, which will be used by the find_dependency() calls
# inside the generated ModuleDependencies cmake files.
install(DIRECTORY cmake/
    DESTINATION "${config_install_dir}"
    FILES_MATCHING PATTERN "Find*.cmake"
    PATTERN "tests" EXCLUDE
    PATTERN "3rdparty" EXCLUDE
)
