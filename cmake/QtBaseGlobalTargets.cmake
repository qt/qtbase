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
install(TARGETS "${name}" EXPORT "${name}${PROJECT_VERSION_MAJOR}Targets")
install(EXPORT "${name}${PROJECT_VERSION_MAJOR}Targets" NAMESPACE Qt:: DESTINATION "${config_install_dir}")

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
add_library(Qt_global_Config INTERFACE)

qt_feature_module_begin(LIBRARY Core
    PUBLIC_FILE src/corelib/global/qconfig.h
    PRIVATE_FILE src/corelib/global/qconfig_p.h
)
include("${CMAKE_CURRENT_SOURCE_DIR}/configure.cmake")
qt_feature_module_end(Qt_global_Config)


## Install some QtBase specific CMake files:
install(FILES
        cmake/QtBuild.cmake
        cmake/QtFeature.cmake
        cmake/QtPostProcess.cmake
        cmake/QtSetup.cmake
        cmake/QtModuleConfig.cmake.in
    DESTINATION "${config_install_dir}"
)
