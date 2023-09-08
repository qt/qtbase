if(NOT QT_INTERNAL_AVOID_OVERRIDING_SYNCQT_CONFIG)
    qt_internal_get_configs_for_flag_manipulation(configs)
    qt_internal_remove_known_optimization_flags(LANGUAGES CXX CONFIGS ${configs})
    # The /RTC1 flag is present in the default DEBUG flags list and contradicts -O2 but is not
    # removed by qt_internal_remove_known_optimization_flags
    qt_internal_remove_compiler_flags("/RTC1" LANGUAGES CXX CONFIGS ${configs})
    qt_internal_get_optimize_full_flags(optimize_full_flags)
    qt_internal_add_compiler_flags(LANGUAGES CXX CONFIGS ${configs} FLAGS "${optimize_full_flags}")

    if(MSVC)
        qt_internal_add_compiler_flags(LANGUAGES CXX CONFIGS ${configs} FLAGS "/EHsc")
    else()
        qt_internal_add_compiler_flags(LANGUAGES CXX CONFIGS ${configs} FLAGS "-fexceptions")
    endif()

    # Replace all linker flags with those we use in the RelWithDebInfo configuration
    list(REMOVE_ITEM configs RELWITHDEBINFO)
    foreach(config IN LISTS configs)
        set(CMAKE_EXE_LINKER_FLAGS_${config} "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO}")
    endforeach()
endif()

set(compile_definitions
    QT_VERSION_STR="${PROJECT_VERSION}"
    QT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    QT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    QT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    QT_NAMESPACE="${QT_NAMESPACE}"
)

qt_get_tool_target_name(target_name syncqt)
if(NOT QT_SYNC_HEADERS_AT_CONFIGURE_TIME)
    qt_internal_add_tool(${target_name}
        DEFINES ${compile_definitions}
        EXCEPTIONS
        TOOLS_TARGET Core
        CORE_LIBRARY None
        INSTALL_DIR "${INSTALL_LIBEXECDIR}"
        SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
    )
else()
    set(config_type "")
    if(NOT QT_INTERNAL_AVOID_OVERRIDING_SYNCQT_CONFIG)
        set(config_type CONFIG RelWithDebInfo)
    endif()

    if(CMAKE_OSX_ARCHITECTURES)
        set(osx_architectures "-DCMAKE_OSX_ARCHITECTURES:STRING=${CMAKE_OSX_ARCHITECTURES}")
    endif()

    if(CMAKE_OSX_SYSROOT)
        set(osx_sysroot "-DCMAKE_OSX_SYSROOT:STRING=${CMAKE_OSX_SYSROOT}")
    endif()
    # Note: configure-time tools reserve the original tool name for the imported executable.
    # To re-build syncqt use 'syncqt_build' target.
    qt_internal_add_configure_time_tool(${target_name}
        DEFINES ${compile_definitions}
        TOOLS_TARGET Core
        INSTALL_DIRECTORY "${INSTALL_LIBEXECDIR}"
        CMAKE_FLAGS
            -DCMAKE_CXX_STANDARD_REQUIRED:BOOL=TRUE
            -DCMAKE_CXX_STANDARD:STRING=17
            # std::filesystem API is only available in macOS 10.15+
            -DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=10.15
            "${osx_architectures}"
            "${osx_sysroot}"
        SOURCES
            "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
        ${config_type}
    )
endif()
