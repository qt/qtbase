# Defines the public Qt::Platform target, which is used by both internal Qt builds as well as
# public Qt consuming projects.
function(qt_internal_setup_public_platform_target)
    ## QtPlatform Target:
    add_library(Platform INTERFACE)
    add_library(Qt::Platform ALIAS Platform)
    target_include_directories(Platform
        INTERFACE
        $<BUILD_INTERFACE:${QT_PLATFORM_DEFINITION_DIR_ABSOLUTE}>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
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

    qt_set_msvc_cplusplus_options(Platform INTERFACE)

    # Propagate minimum C++ 17 via Platform to Qt consumers (apps), after the global features
    # are computed.
    qt_set_language_standards_interface_compile_features(Platform)

    # By default enable utf8 sources for both Qt and Qt consumers. Can be opted out.
    qt_enable_utf8_sources(Platform)

endfunction()
