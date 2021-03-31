# This function creates a CMake target for a generic console or GUI binary.
# Please consider to use a more specific version target like the one created
# by qt_add_test or qt_add_tool below.
function(qt_internal_add_executable name)
    qt_parse_all_arguments(arg "qt_internal_add_executable"
        "${__qt_internal_add_executable_optional_args}"
        "${__qt_internal_add_executable_single_args}"
        "${__qt_internal_add_executable_multi_args}"
        ${ARGN})

    if ("x${arg_OUTPUT_DIRECTORY}" STREQUAL "x")
        set(arg_OUTPUT_DIRECTORY "${QT_BUILD_DIR}/${INSTALL_BINDIR}")
    endif()

    get_filename_component(arg_OUTPUT_DIRECTORY "${arg_OUTPUT_DIRECTORY}"
        ABSOLUTE BASE_DIR "${QT_BUILD_DIR}")

    if ("x${arg_INSTALL_DIRECTORY}" STREQUAL "x")
        set(arg_INSTALL_DIRECTORY "${INSTALL_BINDIR}")
    endif()

    if (ANDROID)
        add_library("${name}" MODULE)
        qt_android_apply_arch_suffix("${name}")
        qt_android_generate_deployment_settings("${name}")
        qt_android_add_apk_target("${name}")
    else()
        add_executable("${name}" ${arg_EXE_FLAGS})
    endif()

    if(arg_QT_APP AND QT_FEATURE_debug_and_release AND CMAKE_VERSION VERSION_GREATER_EQUAL "3.19.0")
        set_property(TARGET "${target}"
            PROPERTY EXCLUDE_FROM_ALL "$<NOT:$<CONFIG:${QT_MULTI_CONFIG_FIRST_CONFIG}>>")
    endif()

    if (arg_VERSION)
        if(arg_VERSION MATCHES "[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+")
            # nothing to do
        elseif(arg_VERSION MATCHES "[0-9]+\\.[0-9]+\\.[0-9]+")
            set(arg_VERSION "${arg_VERSION}.0")
        elseif(arg_VERSION MATCHES "[0-9]+\\.[0-9]+")
            set(arg_VERSION "${arg_VERSION}.0.0")
        elseif (arg_VERSION MATCHES "[0-9]+")
            set(arg_VERSION "${arg_VERSION}.0.0.0")
        else()
            message(FATAL_ERROR "Invalid version format")
        endif()
    endif()

    if(arg_DELAY_TARGET_INFO)
        # Delay the setting of target info properties if requested. Needed for scope finalization
        # of Qt apps.
        set_target_properties("${name}" PROPERTIES
            QT_DELAYED_TARGET_VERSION "${arg_VERSION}"
            QT_DELAYED_TARGET_PRODUCT "${arg_TARGET_PRODUCT}"
            QT_DELAYED_TARGET_DESCRIPTION "${arg_TARGET_DESCRIPTION}"
            QT_DELAYED_TARGET_COMPANY "${arg_TARGET_COMPANY}"
            QT_DELAYED_TARGET_COPYRIGHT "${arg_TARGET_COPYRIGHT}"
            )
    else()
        if("${arg_TARGET_DESCRIPTION}" STREQUAL "")
            set(arg_TARGET_DESCRIPTION "Qt ${name}")
        endif()
        qt_set_target_info_properties(${name} ${ARGN}
            TARGET_DESCRIPTION "${arg_TARGET_DESCRIPTION}"
            TARGET_VERSION "${arg_VERSION}")
    endif()

    if (WIN32 AND NOT arg_DELAY_RC)
        _qt_internal_generate_win32_rc_file(${name})
    endif()

    qt_set_common_target_properties(${name})
    if(ANDROID)
        # On our qmake builds we don't compile the executables with
        # visibility=hidden. Not having this flag set will cause the
        # executable to have main() hidden and can then no longer be loaded
        # through dlopen()
        set_property(TARGET ${name} PROPERTY C_VISIBILITY_PRESET default)
        set_property(TARGET ${name} PROPERTY CXX_VISIBILITY_PRESET default)
    endif()
    qt_autogen_tools_initial_setup(${name})
    qt_skip_warnings_are_errors_when_repo_unclean("${name}")

    set(extra_libraries "")
    if(NOT arg_BOOTSTRAP AND NOT arg_NO_QT)
        set(extra_libraries "Qt::Core")
    endif()

    set(private_includes
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}"
         ${arg_INCLUDE_DIRECTORIES}
    )

    qt_internal_extend_target("${name}"
        SOURCES ${arg_SOURCES}
        INCLUDE_DIRECTORIES ${private_includes}
        DEFINES ${arg_DEFINES}
        LIBRARIES ${arg_LIBRARIES} Qt::PlatformCommonInternal
        PUBLIC_LIBRARIES ${extra_libraries} ${arg_PUBLIC_LIBRARIES}
        DBUS_ADAPTOR_SOURCES "${arg_DBUS_ADAPTOR_SOURCES}"
        DBUS_ADAPTOR_FLAGS "${arg_DBUS_ADAPTOR_FLAGS}"
        DBUS_INTERFACE_SOURCES "${arg_DBUS_INTERFACE_SOURCES}"
        DBUS_INTERFACE_FLAGS "${arg_DBUS_INTERFACE_FLAGS}"
        COMPILE_OPTIONS ${arg_COMPILE_OPTIONS}
        LINK_OPTIONS ${arg_LINK_OPTIONS}
        MOC_OPTIONS ${arg_MOC_OPTIONS}
        ENABLE_AUTOGEN_TOOLS ${arg_ENABLE_AUTOGEN_TOOLS}
        DISABLE_AUTOGEN_TOOLS ${arg_DISABLE_AUTOGEN_TOOLS}
    )
    set_target_properties("${name}" PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${arg_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${arg_OUTPUT_DIRECTORY}"
        WIN32_EXECUTABLE "${arg_GUI}"
        MACOSX_BUNDLE "${arg_GUI}"
    )

    qt_internal_set_exceptions_flags("${name}" ${arg_EXCEPTIONS})

    # Check if target needs to be excluded from all target. Also affects qt_install.
    # Set by qt_exclude_tool_directories_from_default_target.
    set(exclude_from_all FALSE)
    if(__qt_exclude_tool_directories)
        foreach(absolute_dir ${__qt_exclude_tool_directories})
            string(FIND "${CMAKE_CURRENT_SOURCE_DIR}" "${absolute_dir}" dir_starting_pos)
            if(dir_starting_pos EQUAL 0)
                set(exclude_from_all TRUE)
                set_target_properties("${name}" PROPERTIES EXCLUDE_FROM_ALL TRUE)
                break()
            endif()
        endforeach()
    endif()

    if(NOT arg_NO_INSTALL)
        set(additional_install_args "")
        if(exclude_from_all)
            list(APPEND additional_install_args EXCLUDE_FROM_ALL COMPONENT "ExcludedExecutables")
        endif()

        qt_get_cmake_configurations(cmake_configs)
        foreach(cmake_config ${cmake_configs})
            qt_get_install_target_default_args(
                OUT_VAR install_targets_default_args
                CMAKE_CONFIG "${cmake_config}"
                ALL_CMAKE_CONFIGS "${cmake_configs}"
                RUNTIME "${arg_INSTALL_DIRECTORY}"
                LIBRARY "${arg_INSTALL_DIRECTORY}"
                BUNDLE "${arg_INSTALL_DIRECTORY}")

            # Make installation optional for targets that are not built by default in this config
            if(NOT exclude_from_all AND arg_QT_APP AND QT_FEATURE_debug_and_release
                    AND NOT (cmake_config STREQUAL QT_MULTI_CONFIG_FIRST_CONFIG))
                set(install_optional_arg "OPTIONAL")
            else()
                unset(install_optional_arg)
            endif()

            qt_install(TARGETS "${name}"
                       ${additional_install_args} # Needs to be before the DESTINATIONS.
                       ${install_optional_arg}
                       CONFIGURATIONS ${cmake_config}
                       ${install_targets_default_args})
        endforeach()

        qt_enable_separate_debug_info(${name} "${arg_INSTALL_DIRECTORY}")
        qt_internal_install_pdb_files(${name} "${arg_INSTALL_DIRECTORY}")
    endif()

    # If linking against Gui, make sure to also build the default QPA plugin.
    # This makes the experience of an initial Qt configuration to build and run one single
    # test / executable nicer.
    get_target_property(linked_libs "${name}" LINK_LIBRARIES)
    if("Qt::Gui" IN_LIST linked_libs AND TARGET qpa_default_plugins)
        add_dependencies("${name}" qpa_default_plugins)
    endif()

    if(NOT BUILD_SHARED_LIBS)
        # For static builds, we need to explicitly link to plugins we want to be
        # loaded with the executable. User projects get that automatically, but
        # for tools built as part of Qt, we can't use that mechanism because it
        # would pollute the targets we export as part of an install and lead to
        # circular dependencies. The logic here is a simpler equivalent of the
        # more dynamic logic in QtPlugins.cmake.in, but restricted to only
        # adding plugins that are provided by the same module as the module
        # libraries the executable links to.
        set(libs
            ${arg_LIBRARIES}
            ${arg_PUBLIC_LIBRARIES}
            ${extra_libraries}
            Qt::PlatformCommonInternal
        )
        list(REMOVE_DUPLICATES libs)
        foreach(lib IN LISTS libs)
            if(NOT TARGET "${lib}")
                continue()
            endif()
            string(MAKE_C_IDENTIFIER "${name}_plugin_imports_${lib}" out_file)
            string(APPEND out_file .cpp)
            set(class_names "$<GENEX_EVAL:$<TARGET_PROPERTY:${lib},_qt_repo_plugin_class_names>>")
            file(GENERATE OUTPUT ${out_file} CONTENT
"// This file is auto-generated. Do not edit.
#include <QtPlugin>

Q_IMPORT_PLUGIN($<JOIN:${class_names},)\nQ_IMPORT_PLUGIN(>)
"
                CONDITION "$<NOT:$<STREQUAL:${class_names},>>"
            )
            target_sources(${name} PRIVATE
                "$<$<NOT:$<STREQUAL:${class_names},>>:${out_file}>"
            )
            target_link_libraries(${name} PRIVATE "$<TARGET_PROPERTY:${lib},_qt_repo_plugins>")
        endforeach()
    endif()
endfunction()
