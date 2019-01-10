# Where QtBuild.cmake can find QtModuleConfig.cmake.in
set(Qt${PROJECT_VERSION_MAJOR}_DIR "${PROJECT_SOURCE_DIR}/cmake")


## QtPlatform Target:
set(name "Qt")
add_library("${name}" INTERFACE)
add_library("Qt::Platform" ALIAS "${name}")
target_include_directories("${name}"
    INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/${QT_PLATFORM_DEFINITION_DIR}>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:${INSTALL_DATADIR}/${QT_PLATFORM_DEFINITION_DIR}>
    $<INSTALL_INTERFACE:${INSTALL_INCLUDEDIR}>
    )
target_compile_definitions("${name}" INTERFACE ${QT_PLATFORM_DEFINITIONS})
set(config_install_dir "${INSTALL_LIBDIR}/cmake/${name}${PROJECT_VERSION_MAJOR}")

configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/QtConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/Qt${PROJECT_VERSION_MAJOR}Config.cmake"
    INSTALL_DESTINATION "${config_install_dir}"
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/Qt${PROJECT_VERSION_MAJOR}ConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/Qt${PROJECT_VERSION_MAJOR}Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/Qt${PROJECT_VERSION_MAJOR}ConfigVersion.cmake"
    DESTINATION "${config_install_dir}"
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

install(TARGETS "${name}" GlobalConfig GlobalConfigPrivate EXPORT "${name}${PROJECT_VERSION_MAJOR}Targets")
install(EXPORT "${name}${PROJECT_VERSION_MAJOR}Targets" NAMESPACE Qt:: DESTINATION "${config_install_dir}")

## Install some QtBase specific CMake files:
install(FILES
        cmake/QtBuild.cmake
        cmake/QtFeature.cmake
        cmake/QtPostProcess.cmake
        cmake/QtSetup.cmake
        cmake/QtModuleConfig.cmake.in
    DESTINATION "${config_install_dir}"
)
