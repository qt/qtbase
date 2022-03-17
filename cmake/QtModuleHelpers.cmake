macro(qt_internal_get_internal_add_module_keywords option_args single_args multi_args)
    set(${option_args}
        STATIC
        EXCEPTIONS
        INTERNAL_MODULE
        HEADER_MODULE
        DISABLE_TOOLS_EXPORT
        SKIP_DEPENDS_INCLUDE
        NO_MODULE_HEADERS
        NO_SYNC_QT
        NO_PRIVATE_MODULE
        NO_CONFIG_HEADER_FILE
        NO_ADDITIONAL_TARGET_INFO
        NO_GENERATE_METATYPES
        GENERATE_CPP_EXPORTS # TODO: Rename to NO_GENERATE_CPP_EXPORTS once migration is done
        GENERATE_METATYPES          # TODO: Remove once it is not used anymore
        GENERATE_PRIVATE_CPP_EXPORTS
    )
    set(${single_args}
        MODULE_INCLUDE_NAME
        MODULE_INTERFACE_NAME
        CONFIG_MODULE_NAME
        PRECOMPILED_HEADER
        CONFIGURE_FILE_PATH
        CPP_EXPORT_HEADER_BASE_NAME
        EXTERNAL_HEADERS_DIR
        ${__default_target_info_args}
    )
    set(${multi_args}
        QMAKE_MODULE_CONFIG
        EXTRA_CMAKE_FILES
        EXTRA_CMAKE_INCLUDES
        NO_PCH_SOURCES
        EXTERNAL_HEADERS
        ${__default_private_args}
        ${__default_public_args}
        ${__default_private_module_args}
    )
endmacro()

# This is the main entry function for creating a Qt module, that typically
# consists of a library, public header files, private header files and configurable
# features.
#
# A CMake target with the specified target parameter is created. If the current source
# directory has a configure.cmake file, then that is also processed for feature definition
# and testing. Any features defined as well as any features coming from dependencies to
# this module are imported into the scope of the calling feature.
#
# Target is without leading "Qt". So e.g. the "QtCore" module has the target "Core".
#
# Options:
#   NO_ADDITIONAL_TARGET_INFO
#     Don't generate a Qt6*AdditionalTargetInfo.cmake file.
#     The caller is responsible for creating one.
#
#   MODULE_INTERFACE_NAME
#     The custom name of the module interface. This name is used as a part of the include paths
#     associated with the module and other interface names. The default value is the target name.
#     If the INTERNAL_MODULE option is specified, MODULE_INTERFACE_NAME is not specified and the
#     target name ends with the suffix 'Private', the MODULE_INTERFACE_NAME value defaults to the
#     non-suffixed target name, e.g.:
#        For the SomeInternalModulePrivate target, the MODULE_INTERFACE_NAME will be
#        SomeInternalModule
#
#   HEADER_MODULE
#     Creates an interface library instead of following the Qt configuration default. Mutually
#     exclusive with STATIC.
#
#   STATIC
#     Creates a static library instead of following the Qt configuration default. Mutually
#     exclusive with HEADER_MODULE.
#
#   EXTERNAL_HEADERS
#     A explicit list of non qt headers (like 3rdparty) to be installed.
#     Note this option overrides install headers used as PUBLIC_HEADER by cmake install(TARGET)
#     otherwise set by syncqt.
#
#   EXTERNAL_HEADERS_DIR
#     A module directory with non qt headers (like 3rdparty) to be installed.
#     Note this option overrides install headers used as PUBLIC_HEADER by cmake install(TARGET)
#     otherwise set by syncqt.

function(qt_internal_add_module target)
    qt_internal_get_internal_add_module_keywords(
        module_option_args
        module_single_args
        module_multi_args
    )

    qt_parse_all_arguments(arg "qt_internal_add_module"
        "${module_option_args}"
        "${module_single_args}"
        "${module_multi_args}"
        ${ARGN}
    )

    if(arg_INTERNAL_MODULE)
        set(arg_INTERNAL_MODULE "INTERNAL_MODULE")
        set(arg_NO_PRIVATE_MODULE TRUE)
        # Assume the interface name of the internal module should be the module name without the
        # 'Private' suffix.
        if(NOT arg_MODULE_INTERFACE_NAME)
            if(target MATCHES "(.*)Private$")
                set(arg_MODULE_INTERFACE_NAME "${CMAKE_MATCH_1}")
            else()
                message(WARNING "The internal module target should end with the 'Private' suffix.")
            endif()
        endif()
    else()
        unset(arg_INTERNAL_MODULE)
    endif()

    if(NOT arg_MODULE_INTERFACE_NAME)
        set(arg_MODULE_INTERFACE_NAME "${target}")
    endif()

    ### Define Targets:
    if(arg_HEADER_MODULE)
        set(type_to_create INTERFACE)
    elseif(arg_STATIC)
        set(type_to_create STATIC)
    else()
        # Use default depending on Qt configuration.
        set(type_to_create "")
    endif()

    _qt_internal_add_library("${target}" ${type_to_create})
    qt_internal_mark_as_internal_library("${target}")

    get_target_property(target_type ${target} TYPE)

    set(is_interface_lib 0)
    set(is_shared_lib 0)
    set(is_static_lib 0)
    if(target_type STREQUAL "INTERFACE_LIBRARY")
        set(is_interface_lib 1)
    elseif(target_type STREQUAL "STATIC_LIBRARY")
        set(is_static_lib 1)
    elseif(target_type STREQUAL "SHARED_LIBRARY")
        set(is_shared_lib 1)
    else()
        message(FATAL_ERROR "Invalid target type '${target_type}' for Qt module '${target}'")
    endif()

    if(NOT arg_NO_SYNC_QT AND NOT arg_NO_MODULE_HEADERS AND arg_MODULE_INCLUDE_NAME)
        # qt_internal_module_info uses this property if it's set, so it must be
        # specified before the qt_internal_module_info call.
        set_target_properties(${target} PROPERTIES
            _qt_module_include_name ${arg_MODULE_INCLUDE_NAME}
        )
    endif()

    set_target_properties(${target} PROPERTIES
        _qt_module_interface_name "${arg_MODULE_INTERFACE_NAME}"
    )
    set_property(TARGET ${target} APPEND PROPERTY EXPORT_PROPERTIES _qt_module_interface_name)

    qt_internal_module_info(module "${target}")
    qt_internal_add_qt_repo_known_module("${target}")
    if(arg_INTERNAL_MODULE)
        set_target_properties(${target} PROPERTIES _qt_is_internal_module TRUE)
        set_property(TARGET ${target} APPEND PROPERTY EXPORT_PROPERTIES _qt_is_internal_module)
    endif()

    if(NOT arg_CONFIG_MODULE_NAME)
        set(arg_CONFIG_MODULE_NAME "${module_lower}")
    endif()

    set(module_config_header "qt${arg_CONFIG_MODULE_NAME}-config.h")
    set(module_config_private_header "qt${arg_CONFIG_MODULE_NAME}-config_p.h")

    # Module define needs to take into account the config module name.
    string(TOUPPER "${arg_CONFIG_MODULE_NAME}" module_define_infix)
    string(REPLACE "-" "_" module_define_infix "${module_define_infix}")
    string(REPLACE "." "_" module_define_infix "${module_define_infix}")

    set(property_prefix "INTERFACE_")
    if(NOT arg_HEADER_MODULE)
        qt_set_common_target_properties(${target})
        set(property_prefix "")
    endif()

    if(arg_INTERNAL_MODULE)
        string(APPEND arg_CONFIG_MODULE_NAME "_private")
    endif()
    set_target_properties(${target} PROPERTIES
        _qt_config_module_name "${arg_CONFIG_MODULE_NAME}"
        ${property_prefix}QT_QMAKE_MODULE_CONFIG "${arg_QMAKE_MODULE_CONFIG}")
    set_property(TARGET "${target}" APPEND PROPERTY EXPORT_PROPERTIES _qt_config_module_name)

    set(is_framework 0)
    if(QT_FEATURE_framework AND NOT ${arg_HEADER_MODULE} AND NOT ${arg_STATIC})
        set(is_framework 1)
        set_target_properties(${target} PROPERTIES
            FRAMEWORK TRUE
            FRAMEWORK_VERSION "A" # Not based on Qt major version
            MACOSX_FRAMEWORK_IDENTIFIER org.qt-project.${module}
            MACOSX_FRAMEWORK_BUNDLE_VERSION ${PROJECT_VERSION}
            MACOSX_FRAMEWORK_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        )
        qt_internal_get_framework_info(fw ${target})
    endif()

    if(QT_FEATURE_reduce_relocations AND UNIX AND NOT is_interface_lib)
        # On x86 and x86-64 systems with ELF binaries (especially Linux), due to
        # a new optimization in GCC 5.x in combination with a recent version of
        # GNU binutils, compiling Qt applications with -fPIE is no longer
        # enough.
        # Applications now need to be compiled with the -fPIC option if the Qt option
        # \"reduce relocations\" is active.
        target_compile_options(${target} INTERFACE -fPIC)
        if(GCC AND is_shared_lib)
            target_link_options(${target} PRIVATE LINKER:-Bsymbolic-functions)
        endif()
    endif()

    if((FEATURE_ltcg OR CMAKE_INTERPROCEDURAL_OPTIMIZATION) AND GCC AND is_static_lib)
        # CMake <= 3.19 appends -fno-fat-lto-objects for all library types if
        # CMAKE_INTERPROCEDURAL_OPTIMIZATION is enabled. Static libraries need
        # the opposite compiler option.
        # (https://gitlab.kitware.com/cmake/cmake/-/issues/21696)
        target_compile_options(${target} PRIVATE -ffat-lto-objects)
    endif()

    qt_internal_add_target_aliases("${target}")
    qt_skip_warnings_are_errors_when_repo_unclean("${target}")
    _qt_internal_apply_strict_cpp("${target}")

    # No need to compile Q_IMPORT_PLUGIN-containing files for non-executables.
    if(is_static_lib)
        _qt_internal_disable_static_default_plugins("${target}")
    endif()

    # Add _private target to link against the private headers:
    set(target_private "${target}Private")
    if(NOT ${arg_NO_PRIVATE_MODULE})
        add_library("${target_private}" INTERFACE)
        qt_internal_add_target_aliases("${target_private}")
        set_target_properties(${target_private} PROPERTIES
            _qt_config_module_name ${arg_CONFIG_MODULE_NAME}_private)
        set_property(TARGET "${target_private}" APPEND PROPERTY
                     EXPORT_PROPERTIES _qt_config_module_name)
    endif()

    if(NOT arg_HEADER_MODULE)
        set_target_properties(${target} PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY "${QT_BUILD_DIR}/${INSTALL_LIBDIR}"
            RUNTIME_OUTPUT_DIRECTORY "${QT_BUILD_DIR}/${INSTALL_BINDIR}"
            ARCHIVE_OUTPUT_DIRECTORY "${QT_BUILD_DIR}/${INSTALL_LIBDIR}"
            VERSION ${PROJECT_VERSION}
            SOVERSION ${PROJECT_VERSION_MAJOR}
            )
        qt_set_target_info_properties(${target} ${ARGN})
        qt_handle_multi_config_output_dirs("${target}")

        if(NOT BUILD_SHARED_LIBS AND LINUX)
            # Horrible workaround for static build failures due to incorrect static library link
            # order. By increasing the multiplicity to 3, each library cycle will be repeated
            # 3 times on the link line, reducing the probability of undefined symbols at
            # link time.
            # These failures are only observed on Linux with the ld linker (not sure about
            # ld.gold).
            # Allow opting out and modifying the value via cache value,  in case if we urgently
            # need to increase it without waiting for the qtbase change to propagate to
            # other dependent repos.
            # The proper fix will be to get rid of the cycles in the future.
            # See QTBUG-83498 for details.
            set(default_link_cycle_multiplicity "3")
            if(DEFINED QT_LINK_CYCLE_MULTIPLICITY)
                set(default_link_cycle_multiplicity "${QT_LINK_CYCLE_MULTIPLICITY}")
            endif()
            if(default_link_cycle_multiplicity)
                set_property(TARGET "${target}"
                             PROPERTY
                             LINK_INTERFACE_MULTIPLICITY "${default_link_cycle_multiplicity}")
            endif()
        endif()

        if (arg_SKIP_DEPENDS_INCLUDE)
            set_target_properties(${target} PROPERTIES _qt_module_skip_depends_include TRUE)
            set_property(TARGET "${target}" APPEND PROPERTY
                         EXPORT_PROPERTIES _qt_module_skip_depends_include)
        endif()
        if(is_framework)
            set_target_properties(${target} PROPERTIES
                OUTPUT_NAME ${fw_name}
            )
        else()
            set_target_properties(${target} PROPERTIES
                OUTPUT_NAME "${INSTALL_CMAKE_NAMESPACE}${module_interface_name}${QT_LIBINFIX}"
            )
        endif()

        if (WIN32 AND BUILD_SHARED_LIBS)
            _qt_internal_generate_win32_rc_file(${target})
        endif()
    endif()

    # Module headers:
    set_property(TARGET "${target}" APPEND PROPERTY EXPORT_PROPERTIES _qt_module_has_headers)
    if(${arg_NO_MODULE_HEADERS} OR ${arg_NO_SYNC_QT})
        set_target_properties("${target}" PROPERTIES
            _qt_module_has_headers OFF)
    else()
        set_property(TARGET ${target} APPEND PROPERTY EXPORT_PROPERTIES _qt_module_include_name)
        set_target_properties("${target}" PROPERTIES
            _qt_module_include_name "${module_include_name}")

        # Use QT_BUILD_DIR for the syncqt call.
        # So we either write the generated files into the qtbase non-prefix build root, or the
        # module specific build root.
        qt_ensure_sync_qt()
        set(syncqt_full_command "${HOST_PERL}" -w "${QT_SYNCQT}"
                                 -quiet
                                 -check-includes
                                 -module "${module_include_name}"
                                 -version "${PROJECT_VERSION}"
                                 -outdir "${QT_BUILD_DIR}"
                                 -builddir "${PROJECT_BINARY_DIR}"
                                 "${PROJECT_SOURCE_DIR}")
        message(STATUS "Running syncqt for module: '${module_include_name}' ")
        execute_process(COMMAND ${syncqt_full_command} RESULT_VARIABLE syncqt_ret)
        if(NOT syncqt_ret EQUAL 0)
            message(FATAL_ERROR "Failed to run syncqt, return code: ${syncqt_ret}")
        endif()

        set_target_properties("${target}" PROPERTIES
            _qt_module_has_headers ON)

        ### FIXME: Can we replace headers.pri?
        qt_read_headers_pri("${module_build_interface_include_dir}" "module_headers")

        if(arg_EXTERNAL_HEADERS)
            set(module_headers_public ${arg_EXTERNAL_HEADERS})
        endif()

        set_property(TARGET ${target} APPEND PROPERTY
            _qt_module_timestamp_dependencies "${module_headers_public}")

        # We should not generate export headers if module is defined as pure STATIC.
        # Static libraries don't need to export their symbols, and corner cases when sources are
        # also used in shared libraries, should be handled manually.
        if(arg_GENERATE_CPP_EXPORTS AND NOT arg_STATIC)
            if(arg_CPP_EXPORT_HEADER_BASE_NAME)
                set(cpp_export_header_base_name
                    "CPP_EXPORT_HEADER_BASE_NAME;${arg_CPP_EXPORT_HEADER_BASE_NAME}"
                )
            endif()
            if(arg_GENERATE_PRIVATE_CPP_EXPORTS)
                set(generate_private_cpp_export "GENERATE_PRIVATE_CPP_EXPORTS")
            endif()
            qt_internal_generate_cpp_global_exports(${target} ${module_define_infix}
                "${cpp_export_header_base_name}"
                "${generate_private_cpp_export}"
            )
        endif()

        set(module_depends_header
            "${module_build_interface_include_dir}/${module_include_name}Depends")
        if(is_framework)
            if(NOT is_interface_lib)
                set(public_headers_to_copy "${module_headers_public}" "${module_depends_header}")
                qt_copy_framework_headers(${target} PUBLIC "${public_headers_to_copy}")
                qt_copy_framework_headers(${target} PRIVATE "${module_headers_private}")
            endif()
        else()
            set_property(TARGET ${target} APPEND PROPERTY PUBLIC_HEADER "${module_headers_public}")
            set_property(TARGET ${target} APPEND PROPERTY PUBLIC_HEADER ${module_depends_header})
            set_property(TARGET ${target} APPEND PROPERTY PRIVATE_HEADER "${module_headers_private}")
        endif()
        if (NOT ${arg_HEADER_MODULE})
            set_property(TARGET "${target}" PROPERTY MODULE_HEADER
                "${module_build_interface_include_dir}/${module_include_name}")
        endif()

        if(module_headers_qpa)
            if(is_framework)
                qt_copy_framework_headers(${target} QPA "${module_headers_qpa}")
            else()
                qt_install(
                    FILES ${module_headers_qpa}
                    DESTINATION "${module_install_interface_versioned_inner_include_dir}/qpa")
            endif()
        endif()
    endif()

    if(NOT arg_HEADER_MODULE)
        # Plugin types associated to a module
        if(NOT "x${arg_PLUGIN_TYPES}" STREQUAL "x")
            # Reset the variable containing the list of plugins for the given plugin type
            foreach(plugin_type ${arg_PLUGIN_TYPES})
                qt_get_sanitized_plugin_type("${plugin_type}" plugin_type)
                set_property(TARGET "${target}" APPEND PROPERTY MODULE_PLUGIN_TYPES "${plugin_type}")
                qt_internal_add_qt_repo_known_plugin_types("${plugin_type}")
            endforeach()

            # Save the non-sanitized plugin type values for qmake consumption via .pri files.
            set_property(TARGET "${target}"
                         PROPERTY QMAKE_MODULE_PLUGIN_TYPES "${arg_PLUGIN_TYPES}")

            # Export the plugin types.
            set_property(TARGET ${target} APPEND PROPERTY EXPORT_PROPERTIES MODULE_PLUGIN_TYPES)
        endif()
    endif()

    qt_internal_library_deprecation_level(deprecation_define)

    if(NOT arg_HEADER_MODULE)
        qt_autogen_tools_initial_setup(${target})
    endif()

    set(private_includes
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
         ${arg_INCLUDE_DIRECTORIES}
    )

    set(public_includes "")
    set(public_headers_list "public_includes")
    if(is_framework)
        set(public_headers_list "private_includes")
    endif()

    # Make sure the BUILD_INTERFACE include paths come before the framework headers, so that the
    # the compiler prefers the build dir includes.
    #
    # Make sure to add non-framework "build_dir/include" as an include path for moc to find the
    # currently built module headers. qmake does this too.
    # Framework-style include paths are found by moc when cmQtAutoMocUic.cxx detects frameworks by
    # looking at an include path and detecting a "QtFoo.framework/Headers" path.
    # Make sure to create such paths for both the the BUILD_INTERFACE and the INSTALL_INTERFACE.
    #
    # Only add syncqt headers if they exist.
    # This handles cases like QmlDevToolsPrivate which do not have their own headers, but borrow them
    # from another module.
    if(NOT arg_NO_SYNC_QT AND NOT arg_NO_MODULE_HEADERS)
        # Don't include private headers unless they exist, aka syncqt created them.
        if(module_headers_private)
            list(APPEND private_includes
                "$<BUILD_INTERFACE:${module_build_interface_versioned_include_dir}>"
                "$<BUILD_INTERFACE:${module_build_interface_versioned_inner_include_dir}>")
        endif()

        list(APPEND public_includes
                    # For the syncqt headers
                    "$<BUILD_INTERFACE:${repo_build_interface_include_dir}>"
                    "$<BUILD_INTERFACE:${module_build_interface_include_dir}>")
    endif()

    if(is_framework)
        set(fw_install_dir "${INSTALL_LIBDIR}/${fw_dir}")
        set(fw_install_header_dir "${INSTALL_LIBDIR}/${fw_header_dir}")
        set(fw_output_header_dir "${QT_BUILD_DIR}/${fw_install_header_dir}")
        list(APPEND public_includes
            # Add the framework Headers subdir, so that non-framework-style includes work. The
            # BUILD_INTERFACE Headers symlink was previously claimed not to exist at the relevant
            # time, and a fully specified Header path was used instead. This doesn't seem to be a
            # problem anymore.
            "$<BUILD_INTERFACE:${fw_output_header_dir}>"
            "$<INSTALL_INTERFACE:${fw_install_header_dir}>"

            # Add the lib/Foo.framework dir as an include path to let CMake generate
            # the -F compiler flag for framework-style includes to work.
            # Make sure it is added AFTER the lib/Foo.framework/Headers include path,
            # to mitigate issues like QTBUG-101718 and QTBUG-101775 where an include like
            # #include <QtCore> might cause moc to include the QtCore framework shared library
            # instead of the actual header.
            "$<INSTALL_INTERFACE:${fw_install_dir}>"
        )
    endif()

    if(NOT arg_NO_MODULE_HEADERS AND NOT arg_NO_SYNC_QT)
        # For the syncqt headers
        list(APPEND ${public_headers_list}
            "$<INSTALL_INTERFACE:${module_install_interface_include_dir}>")

        # To support finding Qt module includes that are not installed into the main Qt prefix.
        # Use case: A Qt module built by Conan installed into a prefix other than the main prefix.
        # This does duplicate the include path set on Qt6::Platform target, but CMake is smart
        # enough to deduplicate the include paths on the command line.
        # Frameworks are automatically handled by CMake in cmLocalGenerator::GetIncludeFlags()
        # by additionally passing the 'QtFoo.framework/..' dir with an -iframework argument.
        list(APPEND ${public_headers_list} "$<INSTALL_INTERFACE:${INSTALL_INCLUDEDIR}>")
    endif()
    list(APPEND ${public_headers_list} ${arg_PUBLIC_INCLUDE_DIRECTORIES})

    set(header_module)
    if(arg_HEADER_MODULE)
        set(header_module "HEADER_MODULE")

        # Provide a *_timestamp target that can be used to trigger the build of custom_commands.
        set(timestamp_file "${CMAKE_CURRENT_BINARY_DIR}/timestamp")
        add_custom_command(OUTPUT "${timestamp_file}"
            COMMAND ${CMAKE_COMMAND} -E touch "${timestamp_file}"
            DEPENDS "$<TARGET_PROPERTY:${target},_qt_module_timestamp_dependencies>"
            VERBATIM)
        add_custom_target(${target}_timestamp ALL DEPENDS "${timestamp_file}")
    endif()

    set(defines_for_extend_target "")

    if(NOT arg_HEADER_MODULE)
        list(APPEND defines_for_extend_target
            QT_NO_CAST_TO_ASCII QT_ASCII_CAST_WARNINGS
            QT_MOC_COMPAT #we don't need warnings from calling moc code in our generated code
            QT_USE_QSTRINGBUILDER
            QT_DEPRECATED_WARNINGS
            QT_BUILDING_QT
            QT_BUILD_${module_define_infix}_LIB ### FIXME: use QT_BUILD_ADDON for Add-ons or remove if we don't have add-ons anymore
            ${deprecation_define}
            )
        list(APPEND arg_LIBRARIES Qt::PlatformModuleInternal)
    endif()

    qt_internal_extend_target("${target}"
        ${header_module}
        SOURCES ${arg_SOURCES}
        INCLUDE_DIRECTORIES
            ${private_includes}
        PUBLIC_INCLUDE_DIRECTORIES
            ${public_includes}
        PUBLIC_DEFINES
            ${arg_PUBLIC_DEFINES}
        DEFINES
            ${arg_DEFINES}
            ${defines_for_extend_target}
        PUBLIC_LIBRARIES ${arg_PUBLIC_LIBRARIES}
        LIBRARIES ${arg_LIBRARIES}
        PRIVATE_MODULE_INTERFACE ${arg_PRIVATE_MODULE_INTERFACE}
        FEATURE_DEPENDENCIES ${arg_FEATURE_DEPENDENCIES}
        DBUS_ADAPTOR_SOURCES ${arg_DBUS_ADAPTOR_SOURCES}
        DBUS_ADAPTOR_FLAGS ${arg_DBUS_ADAPTOR_FLAGS}
        DBUS_INTERFACE_SOURCES ${arg_DBUS_INTERFACE_SOURCES}
        DBUS_INTERFACE_FLAGS ${arg_DBUS_INTERFACE_FLAGS}
        COMPILE_OPTIONS ${arg_COMPILE_OPTIONS}
        PUBLIC_COMPILE_OPTIONS ${arg_PUBLIC_COMPILE_OPTIONS}
        LINK_OPTIONS ${arg_LINK_OPTIONS}
        PUBLIC_LINK_OPTIONS ${arg_PUBLIC_LINK_OPTIONS}
        MOC_OPTIONS ${arg_MOC_OPTIONS}
        ENABLE_AUTOGEN_TOOLS ${arg_ENABLE_AUTOGEN_TOOLS}
        DISABLE_AUTOGEN_TOOLS ${arg_DISABLE_AUTOGEN_TOOLS}
        PRECOMPILED_HEADER ${arg_PRECOMPILED_HEADER}
        NO_PCH_SOURCES ${arg_NO_PCH_SOURCES}
    )

    # The public module define is not meant to be used when building the module itself,
    # it's only meant to be used for consumers of the module,
    # thus we can't use qt_internal_extend_target()'s PUBLIC_DEFINES option.
    target_compile_definitions(${target} INTERFACE QT_${module_define_infix}_LIB)

    if(NOT arg_EXCEPTIONS AND NOT ${arg_HEADER_MODULE})
        qt_internal_set_exceptions_flags("${target}" FALSE)
    elseif(arg_EXCEPTIONS)
        qt_internal_set_exceptions_flags("${target}" TRUE)
    endif()

    set(configureFile "${CMAKE_CURRENT_SOURCE_DIR}/configure.cmake")
    if(arg_CONFIGURE_FILE_PATH)
        set(configureFile "${arg_CONFIGURE_FILE_PATH}")
    endif()
    if(EXISTS "${configureFile}" AND NOT arg_NO_CONFIG_HEADER_FILE)
        qt_feature_module_begin(
            LIBRARY "${target}"
            PUBLIC_FILE "${module_config_header}"
            PRIVATE_FILE "${module_config_private_header}"
            PUBLIC_DEPENDENCIES ${arg_FEATURE_DEPENDENCIES}
            PRIVATE_DEPENDENCIES ${arg_FEATURE_DEPENDENCIES}
        )
        include(${configureFile})
        qt_feature_module_end("${target}")

        set_property(TARGET "${target}" APPEND PROPERTY
            PUBLIC_HEADER "${CMAKE_CURRENT_BINARY_DIR}/${module_config_header}"
        )
        set_property(TARGET "${target}" APPEND PROPERTY
            PRIVATE_HEADER "${CMAKE_CURRENT_BINARY_DIR}/${module_config_private_header}"
        )
    endif()

    if(NOT arg_HEADER_MODULE)
        if(DEFINED module_headers_private)
            qt_internal_add_linker_version_script("${target}" PRIVATE_HEADERS ${module_headers_private} ${module_headers_qpa})
        else()
            qt_internal_add_linker_version_script("${target}")
        endif()
    endif()

    # Handle injections. Aka create forwarding headers for certain headers that have been
    # automatically generated in the build dir (for example qconfig.h, qtcore-config.h,
    # qvulkanfunctions.h, etc)
    # module_headers_injections come from the qt_read_headers_pri() call.
    # extra_library_injections come from the qt_feature_module_end() call.
    set(final_injections "")
    if(module_headers_injections)
        string(APPEND final_injections "${module_headers_injections} ")
    endif()
    if(extra_library_injections)
        string(APPEND final_injections "${extra_library_injections} ")
    endif()

    if(final_injections)
        qt_install_injections(${target} "${QT_BUILD_DIR}" "${QT_INSTALL_DIR}" ${final_injections})
    endif()

    # Handle creation of cmake files for consumers of find_package().
    set(path_suffix "${INSTALL_CMAKE_NAMESPACE}${target}")
    qt_path_join(config_build_dir ${QT_CONFIG_BUILD_DIR} ${path_suffix})
    qt_path_join(config_install_dir ${QT_CONFIG_INSTALL_DIR} ${path_suffix})

    set(extra_cmake_files)
    set(extra_cmake_includes)
    if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/${INSTALL_CMAKE_NAMESPACE}${target}Macros.cmake")
        list(APPEND extra_cmake_files "${CMAKE_CURRENT_LIST_DIR}/${INSTALL_CMAKE_NAMESPACE}${target}Macros.cmake")
        list(APPEND extra_cmake_includes "${INSTALL_CMAKE_NAMESPACE}${target}Macros.cmake")
    endif()
    if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/${INSTALL_CMAKE_NAMESPACE}${target}ConfigExtras.cmake.in")
        if(target STREQUAL Core)
            set(extra_cmake_code "")
            # Add some variables for compatibility with Qt5 config files.
            if(QT_FEATURE_reduce_exports)
                string(APPEND qtcore_extra_cmake_code "
set(QT_VISIBILITY_AVAILABLE TRUE)")
            endif()
            if(QT_LIBINFIX)
                string(APPEND qtcore_extra_cmake_code "
set(QT_LIBINFIX \"${QT_LIBINFIX}\")")
            endif()

        endif()

        configure_file("${CMAKE_CURRENT_LIST_DIR}/${INSTALL_CMAKE_NAMESPACE}${target}ConfigExtras.cmake.in"
            "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}ConfigExtras.cmake"
            @ONLY)
        list(APPEND extra_cmake_files "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}ConfigExtras.cmake")
        list(APPEND extra_cmake_includes "${INSTALL_CMAKE_NAMESPACE}${target}ConfigExtras.cmake")
    endif()

    foreach(cmake_file IN LISTS arg_EXTRA_CMAKE_FILES)
        get_filename_component(basename ${cmake_file} NAME)
        file(COPY ${cmake_file} DESTINATION ${config_build_dir})
        list(APPEND extra_cmake_files "${config_build_dir}/${basename}")
    endforeach()
    list(APPEND extra_cmake_includes ${arg_EXTRA_CMAKE_INCLUDES})

    set(extra_cmake_code "")

    # Generate metatypes
    if(${arg_GENERATE_METATYPES})
        # No mention of NO_GENERATE_METATYPES. You should not use it.
        message(WARNING "GENERATE_METATYPES is on by default for Qt modules. Please remove the manual specification.")
    endif()
    if (NOT ${arg_NO_GENERATE_METATYPES})
        if (NOT target_type STREQUAL "INTERFACE_LIBRARY")
            set(args "")
            if(QT_WILL_INSTALL)
                set(metatypes_install_dir "${INSTALL_LIBDIR}/metatypes")
                list(APPEND args
                    __QT_INTERNAL_INSTALL __QT_INTERNAL_INSTALL_DIR "${metatypes_install_dir}")
            endif()
            qt6_extract_metatypes(${target} ${args})
        elseif(${arg_GENERATE_METATYPES})
            message(FATAL_ERROR "Meta types generation does not work on interface libraries")
        endif()
    endif()
    qt_internal_get_min_new_policy_cmake_version(min_new_policy_version)
    qt_internal_get_max_new_policy_cmake_version(max_new_policy_version)
    configure_package_config_file(
        "${QT_CMAKE_DIR}/QtModuleConfig.cmake.in"
        "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}Config.cmake"
        INSTALL_DESTINATION "${config_install_dir}"
    )

    if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/${INSTALL_CMAKE_NAMESPACE}${target}BuildInternals.cmake")
        configure_file("${CMAKE_CURRENT_LIST_DIR}/${INSTALL_CMAKE_NAMESPACE}${target}BuildInternals.cmake"
            "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}BuildInternals.cmake"
            @ONLY)
        list(APPEND extra_cmake_files "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}BuildInternals.cmake")
    endif()

    write_basic_package_version_file(
        "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}ConfigVersionImpl.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
    )
    qt_internal_write_qt_package_version_file(
        "${INSTALL_CMAKE_NAMESPACE}${target}"
        "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}ConfigVersion.cmake"
    )
    qt_install(FILES
        "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}Config.cmake"
        "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}ConfigVersion.cmake"
        "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}ConfigVersionImpl.cmake"
        ${extra_cmake_files}
        DESTINATION "${config_install_dir}"
        COMPONENT Devel
    )

    file(COPY ${extra_cmake_files} DESTINATION "${config_build_dir}")
    set(exported_targets ${target})
    if(NOT ${arg_NO_PRIVATE_MODULE})
        list(APPEND exported_targets ${target_private})
    endif()
    set(export_name "${INSTALL_CMAKE_NAMESPACE}${target}Targets")
    if(arg_EXTERNAL_HEADERS_DIR)
        qt_install(DIRECTORY "${arg_EXTERNAL_HEADERS_DIR}/"
            DESTINATION "${module_install_interface_include_dir}"
        )
        unset(public_header_destination)
    else()
        set(public_header_destination PUBLIC_HEADER DESTINATION "${module_install_interface_include_dir}")
    endif()

    qt_install(TARGETS ${exported_targets}
        EXPORT ${export_name}
        RUNTIME DESTINATION ${INSTALL_BINDIR}
        LIBRARY DESTINATION ${INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${INSTALL_LIBDIR}
        FRAMEWORK DESTINATION ${INSTALL_LIBDIR}
        PRIVATE_HEADER DESTINATION "${module_install_interface_private_include_dir}"
        ${public_header_destination}
        )

    if(BUILD_SHARED_LIBS)
        qt_apply_rpaths(TARGET "${target}" INSTALL_PATH "${INSTALL_LIBDIR}" RELATIVE_RPATH)
    endif()

    if (ANDROID AND NOT arg_HEADER_MODULE)
        # Record install library location so it can be accessed by
        # qt_internal_android_dependencies without having to specify it again.
        set_target_properties(${target} PROPERTIES
            QT_ANDROID_MODULE_INSTALL_DIR ${INSTALL_LIBDIR})
    endif()

    qt_install(EXPORT ${export_name}
               NAMESPACE ${QT_CMAKE_EXPORT_NAMESPACE}::
               DESTINATION ${config_install_dir})

    if(NOT arg_NO_ADDITIONAL_TARGET_INFO)
        qt_internal_export_additional_targets_file(
            TARGETS ${exported_targets}
            EXPORT_NAME_PREFIX ${INSTALL_CMAKE_NAMESPACE}${target}
            CONFIG_INSTALL_DIR "${config_install_dir}")
    endif()

    qt_internal_export_modern_cmake_config_targets_file(
        TARGETS ${exported_targets}
        EXPORT_NAME_PREFIX ${INSTALL_CMAKE_NAMESPACE}${target}
        CONFIG_INSTALL_DIR "${config_install_dir}")

    ### fixme: cmake is missing a built-in variable for this. We want to apply it only to modules and plugins
    # that belong to Qt.
    if(NOT arg_HEADER_MODULE)
        qt_internal_add_link_flags_no_undefined("${target}")
    endif()

    set(interface_includes "")

    # Handle cases like QmlDevToolsPrivate which do not have their own headers, but rather borrow them
    # from another module.
    if(NOT arg_NO_SYNC_QT)
        list(APPEND interface_includes "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>")

        # syncqt.pl does not create a private header directory like 'include/6.0/QtFoo' unless
        # the module has foo_p.h header files. For QtZlib, there are no such private headers, so we
        # need to make sure not to add such include paths unless the directory exists, otherwise
        # consumers of the module will fail at CMake generation time stating that
        # INTERFACE_INCLUDE_DIRECTORIES contains a non-existent path.
        if(NOT arg_NO_MODULE_HEADERS
                AND EXISTS "${module_build_interface_versioned_inner_include_dir}")
            list(APPEND interface_includes
                "$<BUILD_INTERFACE:${module_build_interface_versioned_include_dir}>"
                "$<BUILD_INTERFACE:${module_build_interface_versioned_inner_include_dir}>")

            if(is_framework)
                set(fw_install_private_header_dir "${INSTALL_LIBDIR}/${fw_private_header_dir}")
                set(fw_install_private_module_header_dir "${INSTALL_LIBDIR}/${fw_private_module_header_dir}")
                list(APPEND interface_includes
                            "$<INSTALL_INTERFACE:${fw_install_private_header_dir}>"
                            "$<INSTALL_INTERFACE:${fw_install_private_module_header_dir}>")
            else()
                list(APPEND interface_includes
                    "$<INSTALL_INTERFACE:${module_install_interface_versioned_include_dir}>"
                    "$<INSTALL_INTERFACE:${module_install_interface_versioned_inner_include_dir}>")
            endif()
        endif()
    endif()

    if(QT_FEATURE_headersclean AND NOT arg_NO_MODULE_HEADERS)
        qt_internal_add_headers_clean_target(
            ${target}
            "${module_include_name}"
            "${module_headers_clean}")
    endif()

    if(arg_INTERNAL_MODULE)
        target_include_directories("${target}" INTERFACE ${interface_includes})
    elseif(NOT ${arg_NO_PRIVATE_MODULE})
        target_include_directories("${target_private}" INTERFACE ${interface_includes})
        target_link_libraries("${target_private}" INTERFACE "${target}")
    endif()

    set(debug_install_dir "${INSTALL_LIBDIR}")
    if (MINGW)
        set(debug_install_dir "${INSTALL_BINDIR}")
    endif()
    qt_enable_separate_debug_info(${target} "${debug_install_dir}")
    set(pdb_install_dir "${INSTALL_BINDIR}")
    if(NOT is_shared_lib)
        set(pdb_install_dir "${INSTALL_LIBDIR}")
    endif()
    qt_internal_install_pdb_files(${target} "${pdb_install_dir}")

    if (arg_NO_PRIVATE_MODULE)
        set(arg_NO_PRIVATE_MODULE "NO_PRIVATE_MODULE")
    else()
        unset(arg_NO_PRIVATE_MODULE)
    endif()

    qt_describe_module(${target})
    qt_add_list_file_finalizer(qt_finalize_module ${target} ${arg_INTERNAL_MODULE} ${arg_NO_PRIVATE_MODULE} ${header_module})
endfunction()

function(qt_finalize_module target)
    qt_finalize_framework_headers_copy(${target})
    qt_generate_prl_file(${target} "${INSTALL_LIBDIR}")
    qt_generate_module_pri_file("${target}" ${ARGN})
endfunction()

# Get a set of Qt module related values based on the target.
#
# The function uses the _qt_module_interface_name and _qt_module_include_name target properties to
# preform values for the output variables. _qt_module_interface_name it's the basic name of module
# without "Qtfication" and the "Private" suffix if we speak about INTERNAL_MODULEs. Typical value of
# the _qt_module_interface_name is the provided to qt_internal_add_module ${target} name, e.g. Core.
# _qt_module_interface_name is used to preform all the include paths unless the
# _qt_module_include_name property is specified. _qt_module_include_name is legacy property that
# replaces the module name in include paths and has a higher priority than the
# _qt_module_interface_name property.
#
# When doing qt_internal_module_info(foo Core) this method will set the following variables in
# the caller's scope:
#  * foo with the value "QtCore"
#  * foo_versioned with the value "Qt6Core" (based on major Qt version)
#  * foo_upper with the value "CORE"
#  * foo_lower with the value "core"
#  * foo_include_name with the value"QtCore"
#    Usually the module name from ${foo} is used, but the name might be different if the
#    MODULE_INCLUDE_NAME argument is set when creating the module.
#  * foo_versioned_include_dir with the value "QtCore/6.2.0"
#  * foo_versioned_inner_include_dir with the value "QtCore/6.2.0/QtCore"
#  * foo_private_include_dir with the value "QtCore/6.2.0/QtCore/private"
#  * foo_interface_name the interface name of the module stored in _qt_module_interface_name
#    property, e.g. Core.
#
# The function also sets a bunch of module include paths for the build and install interface.
# Variables that contains these paths start with foo_build_interface_ and foo_install_interface_
# accordingly.
# The following variables are set in the caller's scope:
#  * foo_<build|install>_interface_include_dir with
#    qtbase_build_dir/include/QtCore for build interface and
#    include/QtCore for install interface.
#  * foo_<build|install>_interface_versioned_include_dir with
#    qtbase_build_dir/include/QtCore/6.2.0 for build interface and
#    include/QtCore/6.2.0 for install interface.
#  * foo_<build|install>_versioned_inner_include_dir with
#    qtbase_build_dir/include/QtCore/6.2.0/QtCore for build interface and
#    include/QtCore/6.2.0/QtCore for install interface.
#  * foo_<build|install>_private_include_dir with
#    qtbase_build_dir/include/QtCore/6.2.0/QtCore/private for build interface and
#    include/QtCore/6.2.0/QtCore/private for install interface.
# The following values are set by the function and might be useful in caller's scope:
#  * repo_install_interface_include_dir contains path to the top-level repository include directory,
#    e.g. qtbase_build_dir/include
#  * repo_install_interface_include_dir contains path to the non-prefixed top-level include
#    directory is used for the installation, e.g. include
# Note: that for non-prefixed Qt configurations the build interface paths will start with
# <build_directory>/qtbase/include, e.g foo_build_interface_include_dir of the Qml module looks
# like qt_toplevel_build_dir/qtbase/include/QtQml
function(qt_internal_module_info result target)
    if(result STREQUAL "repo")
        message(FATAL_ERROR "'repo' keyword is reserved for internal use, please specify \
the different base name for the module info variables.")
    endif()

    get_target_property(module_interface_name ${target} _qt_module_interface_name)
    if(NOT module_interface_name)
        message(FATAL_ERROR "${target} is not a module.")
    endif()

    qt_internal_qtfy_target(module ${module_interface_name})

    get_target_property("${result}_include_name" ${target} _qt_module_include_name)
    if(NOT ${result}_include_name)
        set("${result}_include_name" "${module}")
    endif()

    set("${result}_versioned_include_dir"
        "${${result}_include_name}/${PROJECT_VERSION}")
    set("${result}_versioned_inner_include_dir"
        "${${result}_versioned_include_dir}/${${result}_include_name}")
    set("${result}_private_include_dir"
        "${${result}_versioned_inner_include_dir}/private")

    # Module build interface directories
    set(repo_build_interface_include_dir "${QT_BUILD_DIR}/include")
    set("${result}_build_interface_include_dir"
        "${repo_build_interface_include_dir}/${${result}_include_name}")
    set("${result}_build_interface_versioned_include_dir"
        "${repo_build_interface_include_dir}/${${result}_versioned_include_dir}")
    set("${result}_build_interface_versioned_inner_include_dir"
        "${repo_build_interface_include_dir}/${${result}_versioned_inner_include_dir}")
    set("${result}_build_interface_private_include_dir"
        "${repo_build_interface_include_dir}/${${result}_private_include_dir}")

    # Module install interface direcotries
    set(repo_install_interface_include_dir "${INSTALL_INCLUDEDIR}")
    set("${result}_install_interface_include_dir"
        "${repo_install_interface_include_dir}/${${result}_include_name}")
    set("${result}_install_interface_versioned_include_dir"
        "${repo_install_interface_include_dir}/${${result}_versioned_include_dir}")
    set("${result}_install_interface_versioned_inner_include_dir"
        "${repo_install_interface_include_dir}/${${result}_versioned_inner_include_dir}")
    set("${result}_install_interface_private_include_dir"
        "${repo_install_interface_include_dir}/${${result}_private_include_dir}")

    set("${result}" "${module}" PARENT_SCOPE)
    set("${result}_versioned" "${module_versioned}" PARENT_SCOPE)
    string(TOUPPER "${module_interface_name}" upper)
    string(TOLOWER "${module_interface_name}" lower)
    set("${result}_upper" "${upper}" PARENT_SCOPE)
    set("${result}_lower" "${lower}" PARENT_SCOPE)
    set("${result}_include_name" "${${result}_include_name}" PARENT_SCOPE)
    set("${result}_versioned_include_dir" "${${result}_versioned_include_dir}" PARENT_SCOPE)
    set("${result}_versioned_inner_include_dir"
        "${${result}_versioned_inner_include_dir}" PARENT_SCOPE)
    set("${result}_private_include_dir" "${${result}_private_include_dir}" PARENT_SCOPE)
    set("${result}_interface_name" "${module_interface_name}" PARENT_SCOPE)

    # Setting module build interface directories in parent scope
    set(repo_build_interface_include_dir "${repo_build_interface_include_dir}" PARENT_SCOPE)
    set("${result}_build_interface_include_dir"
        "${${result}_build_interface_include_dir}" PARENT_SCOPE)
    set("${result}_build_interface_versioned_include_dir"
        "${${result}_build_interface_versioned_include_dir}" PARENT_SCOPE)
    set("${result}_build_interface_versioned_inner_include_dir"
        "${${result}_build_interface_versioned_inner_include_dir}" PARENT_SCOPE)
    set("${result}_build_interface_private_include_dir"
        "${${result}_build_interface_private_include_dir}" PARENT_SCOPE)

    # Setting module install interface directories in parent scope
    set(repo_install_interface_include_dir "${repo_install_interface_include_dir}" PARENT_SCOPE)
    set("${result}_install_interface_include_dir"
        "${${result}_install_interface_include_dir}" PARENT_SCOPE)
    set("${result}_install_interface_versioned_include_dir"
        "${${result}_install_interface_versioned_include_dir}" PARENT_SCOPE)
    set("${result}_install_interface_versioned_inner_include_dir"
        "${${result}_install_interface_versioned_inner_include_dir}" PARENT_SCOPE)
    set("${result}_install_interface_private_include_dir"
        "${${result}_install_interface_private_include_dir}" PARENT_SCOPE)
endfunction()

# Generate a module description file based on the template in ModuleDescription.json.in
function(qt_describe_module target)
    set(path_suffix "${INSTALL_DESCRIPTIONSDIR}")
    qt_path_join(build_dir ${QT_BUILD_DIR} ${path_suffix})
    qt_path_join(install_dir ${QT_INSTALL_DIR} ${path_suffix})

    set(descfile_in "${QT_CMAKE_DIR}/ModuleDescription.json.in")
    set(descfile_out "${build_dir}/${target}.json")
    set(cross_compilation "false")
    if(CMAKE_CROSSCOMPILING)
        set(cross_compilation "true")
    endif()
    configure_file("${descfile_in}" "${descfile_out}")

    qt_install(FILES "${descfile_out}" DESTINATION "${install_dir}")
endfunction()

function(qt_internal_generate_cpp_global_exports target module_define_infix)
    cmake_parse_arguments(arg
        "GENERATE_PRIVATE_CPP_EXPORTS"
        "CPP_EXPORT_HEADER_BASE_NAME"
        "" ${ARGN}
    )

    qt_internal_module_info(module "${target}")

    set(header_base_name "qt${module_lower}exports")
    if(arg_CPP_EXPORT_HEADER_BASE_NAME)
        set(header_base_name "${arg_CPP_EXPORT_HEADER_BASE_NAME}")
    endif()
    # Is used as a part of the header guard define.
    string(TOUPPER "${header_base_name}" header_base_name_upper)

    set(generated_header_path
        "${module_build_interface_include_dir}/${header_base_name}.h"
    )

    configure_file("${QT_CMAKE_DIR}/modulecppexports.h.in"
        "${generated_header_path}" @ONLY
    )

    set(${out_public_header} "${generated_header_path}" PARENT_SCOPE)
    target_sources(${target} PRIVATE "${generated_header_path}")

    if(arg_GENERATE_PRIVATE_CPP_EXPORTS)
        set(generated_private_header_path
            "${module_build_interface_private_include_dir}/${header_base_name}_p.h"
        )

        configure_file("${QT_CMAKE_DIR}/modulecppexports_p.h.in"
            "${generated_private_header_path}" @ONLY
        )

        set(${out_private_header} "${generated_private_header_path}" PARENT_SCOPE)
        target_sources(${target} PRIVATE "${generated_private_header_path}")
    endif()

    get_target_property(is_framework ${target} FRAMEWORK)

    get_target_property(target_type ${target} TYPE)
    set(is_interface_lib 0)
    if(target_type STREQUAL "INTERFACE_LIBRARY")
        set(is_interface_lib 1)
    endif()

    set_property(TARGET ${target} APPEND PROPERTY
        _qt_module_timestamp_dependencies "${generated_header_path}")

    if(is_framework)
        if(NOT is_interface_lib)
            qt_copy_framework_headers(${target} PUBLIC "${generated_header_path}")

            if(arg_GENERATE_PRIVATE_CPP_EXPORTS)
                qt_copy_framework_headers(${target} PRIVATE "${generated_private_header_path}")
            endif()
        endif()
    else()
        set_property(TARGET ${target} APPEND PROPERTY PUBLIC_HEADER "${generated_header_path}")
        qt_install(FILES "${generated_header_path}"
            DESTINATION "${module_install_interface_include_dir}")

        if(arg_GENERATE_PRIVATE_CPP_EXPORTS)
            set_property(TARGET ${target} APPEND PROPERTY PRIVATE_HEADER
                "${generated_private_header_path}")
            qt_install(FILES "${generated_private_header_path}"
                DESTINATION "${module_install_interface_private_include_dir}")
        endif()
    endif()
endfunction()
