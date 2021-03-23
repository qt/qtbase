# Wrapper function to create a regular cmake target and forward all the
# arguments collected by the conversion script.
function(qt_internal_add_cmake_library target)
    # Process arguments:
    qt_parse_all_arguments(arg "qt_add_cmake_library"
        "SHARED;MODULE;STATIC;INTERFACE"
        "OUTPUT_DIRECTORY;ARCHIVE_INSTALL_DIRECTORY;INSTALL_DIRECTORY"
        "${__default_private_args};${__default_public_args}"
        ${ARGN}
    )

    set(is_static_lib 0)

    ### Define Targets:
    if(${arg_INTERFACE})
        add_library("${target}" INTERFACE)
    elseif(${arg_STATIC} OR (${arg_MODULE} AND NOT BUILD_SHARED_LIBS))
        add_library("${target}" STATIC)
        set(is_static_lib 1)
    elseif(${arg_SHARED})
        add_library("${target}" SHARED)
        _qt_internal_apply_win_prefix_and_suffix("${target}")
    elseif(${arg_MODULE})
        add_library("${target}" MODULE)
        set_property(TARGET ${name} PROPERTY C_VISIBILITY_PRESET default)
        set_property(TARGET ${name} PROPERTY CXX_VISIBILITY_PRESET default)
        set_property(TARGET ${name} PROPERTY OBJC_VISIBILITY_PRESET default)
        set_property(TARGET ${name} PROPERTY OBJCXX_VISIBILITY_PRESET default)

        if(APPLE)
            # CMake defaults to using .so extensions for loadable modules, aka plugins,
            # but Qt plugins are actually suffixed with .dylib.
            set_property(TARGET "${target}" PROPERTY SUFFIX ".dylib")
        endif()
        _qt_internal_apply_win_prefix_and_suffix("${target}")
    else()
        add_library("${target}")
        if(NOT BUILD_SHARED_LIBS)
            set(is_static_lib 1)
        endif()
    endif()

    if (NOT arg_ARCHIVE_INSTALL_DIRECTORY AND arg_INSTALL_DIRECTORY)
        set(arg_ARCHIVE_INSTALL_DIRECTORY "${arg_INSTALL_DIRECTORY}")
    endif()

    if (ANDROID)
        qt_android_apply_arch_suffix("${target}")
    endif()
    qt_skip_warnings_are_errors_when_repo_unclean("${target}")

    # No need to compile Q_IMPORT_PLUGIN-containing files for non-executables.
    if(is_static_lib)
        _qt_internal_disable_static_default_plugins("${target}")
    endif()

    if (arg_INSTALL_DIRECTORY)
        set(install_arguments
            ARCHIVE_INSTALL_DIRECTORY ${arg_ARCHIVE_INSTALL_DIRECTORY}
            INSTALL_DIRECTORY ${arg_INSTALL_DIRECTORY}
        )
    endif()

    if (arg_OUTPUT_DIRECTORY)
        set_target_properties(${target} PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY ${arg_OUTPUT_DIRECTORY}
            RUNTIME_OUTPUT_DIRECTORY ${arg_OUTPUT_DIRECTORY}
            LIBRARY_OUTPUT_DIRECTORY ${arg_OUTPUT_DIRECTORY}
        )
    endif()

    qt_internal_extend_target("${target}"
        SOURCES ${arg_SOURCES}
        INCLUDE_DIRECTORIES
            ${arg_INCLUDE_DIRECTORIES}
        PUBLIC_INCLUDE_DIRECTORIES
            ${arg_PUBLIC_INCLUDE_DIRECTORIES}
        PUBLIC_DEFINES
            ${arg_PUBLIC_DEFINES}
        DEFINES
            ${arg_DEFINES}
        PUBLIC_LIBRARIES ${arg_PUBLIC_LIBRARIES}
        LIBRARIES ${arg_LIBRARIES} Qt::PlatformCommonInternal
        COMPILE_OPTIONS ${arg_COMPILE_OPTIONS}
        PUBLIC_COMPILE_OPTIONS ${arg_PUBLIC_COMPILE_OPTIONS}
        LINK_OPTIONS ${arg_LINK_OPTIONS}
        PUBLIC_LINK_OPTIONS ${arg_PUBLIC_LINK_OPTIONS}
        MOC_OPTIONS ${arg_MOC_OPTIONS}
        ENABLE_AUTOGEN_TOOLS ${arg_ENABLE_AUTOGEN_TOOLS}
        DISABLE_AUTOGEN_TOOLS ${arg_DISABLE_AUTOGEN_TOOLS}
        ${install_arguments}
    )
endfunction()

# This function replaces qmake's qt_helper_lib feature. It is intended to
# compile 3rdparty libraries as part of the build.
#
function(qt_internal_add_3rdparty_library target)
    # Process arguments:
    qt_parse_all_arguments(arg "qt_add_3rdparty_library"
        "SHARED;MODULE;STATIC;INTERFACE;EXCEPTIONS;INSTALL;SKIP_AUTOMOC"
        "OUTPUT_DIRECTORY;QMAKE_LIB_NAME"
        "${__default_private_args};${__default_public_args}"
        ${ARGN}
    )

    set(is_static_lib 0)

    ### Define Targets:
    if(${arg_INTERFACE})
        add_library("${target}" INTERFACE)
    elseif(${arg_STATIC} OR (${arg_MODULE} AND NOT BUILD_SHARED_LIBS))
        add_library("${target}" STATIC)
        set(is_static_lib 1)
    elseif(${arg_SHARED})
        add_library("${target}" SHARED)
    elseif(${arg_MODULE})
        add_library("${target}" MODULE)
        set_property(TARGET ${name} PROPERTY C_VISIBILITY_PRESET default)
        set_property(TARGET ${name} PROPERTY CXX_VISIBILITY_PRESET default)
        set_property(TARGET ${name} PROPERTY OBJC_VISIBILITY_PRESET default)
        set_property(TARGET ${name} PROPERTY OBJCXX_VISIBILITY_PRESET default)

        if(APPLE)
            # CMake defaults to using .so extensions for loadable modules, aka plugins,
            # but Qt plugins are actually suffixed with .dylib.
            set_property(TARGET "${target}" PROPERTY SUFFIX ".dylib")
        endif()
    else()
        add_library("${target}")
        if(NOT BUILD_SHARED_LIBS)
            set(is_static_lib 1)
        endif()
    endif()

    if(NOT arg_INTERFACE)
        qt_set_common_target_properties(${target})
    endif()

    if (NOT arg_ARCHIVE_INSTALL_DIRECTORY AND arg_INSTALL_DIRECTORY)
        set(arg_ARCHIVE_INSTALL_DIRECTORY "${arg_INSTALL_DIRECTORY}")
    endif()

    qt_internal_add_qt_repo_known_module(${target})
    qt_internal_add_target_aliases(${target})
    _qt_internal_apply_strict_cpp(${target})

    # No need to compile Q_IMPORT_PLUGIN-containing files for non-executables.
    if(is_static_lib)
        _qt_internal_disable_static_default_plugins("${target}")
    endif()

    if (ANDROID)
        qt_android_apply_arch_suffix("${target}")
    endif()

    qt_skip_warnings_are_errors_when_repo_unclean("${target}")

    set_target_properties(${target} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${QT_BUILD_DIR}/${INSTALL_LIBDIR}"
        RUNTIME_OUTPUT_DIRECTORY "${QT_BUILD_DIR}/${INSTALL_BINDIR}"
        ARCHIVE_OUTPUT_DIRECTORY "${QT_BUILD_DIR}/${INSTALL_LIBDIR}"
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        QT_MODULE_IS_3RDPARTY_LIBRARY TRUE
        QT_MODULE_SKIP_DEPENDS_INCLUDE TRUE
    )
    qt_handle_multi_config_output_dirs("${target}")

    set_target_properties(${target} PROPERTIES
        OUTPUT_NAME "${INSTALL_CMAKE_NAMESPACE}${target}"
    )

    if(NOT arg_SKIP_AUTOMOC)
        qt_autogen_tools_initial_setup(${target})
    endif()

    if(NOT arg_EXCEPTIONS AND NOT arg_INTERFACE)
        qt_internal_set_exceptions_flags("${target}" FALSE)
    elseif(arg_EXCEPTIONS)
        qt_internal_set_exceptions_flags("${target}" TRUE)
    endif()

    qt_internal_extend_target("${target}"
        SOURCES ${arg_SOURCES}
        INCLUDE_DIRECTORIES
            ${arg_INCLUDE_DIRECTORIES}
        PUBLIC_INCLUDE_DIRECTORIES
            ${arg_PUBLIC_INCLUDE_DIRECTORIES}
        PUBLIC_DEFINES
            ${arg_PUBLIC_DEFINES}
        DEFINES
            ${arg_DEFINES}
        PUBLIC_LIBRARIES ${arg_PUBLIC_LIBRARIES}
        LIBRARIES ${arg_LIBRARIES} Qt::PlatformModuleInternal
        COMPILE_OPTIONS ${arg_COMPILE_OPTIONS}
        PUBLIC_COMPILE_OPTIONS ${arg_PUBLIC_COMPILE_OPTIONS}
        LINK_OPTIONS ${arg_LINK_OPTIONS}
        PUBLIC_LINK_OPTIONS ${arg_PUBLIC_LINK_OPTIONS}
        MOC_OPTIONS ${arg_MOC_OPTIONS}
        ENABLE_AUTOGEN_TOOLS ${arg_ENABLE_AUTOGEN_TOOLS}
        DISABLE_AUTOGEN_TOOLS ${arg_DISABLE_AUTOGEN_TOOLS}
        ${install_arguments}
    )

    if(NOT BUILD_SHARED_LIBS OR arg_INSTALL)
        qt_generate_3rdparty_lib_pri_file("${target}" "${arg_QMAKE_LIB_NAME}" pri_file)
        if(pri_file)
            qt_install(FILES "${pri_file}" DESTINATION "${INSTALL_MKSPECSDIR}/modules")
        endif()

        set(path_suffix "${INSTALL_CMAKE_NAMESPACE}${target}")
        qt_path_join(config_build_dir ${QT_CONFIG_BUILD_DIR} ${path_suffix})
        qt_path_join(config_install_dir ${QT_CONFIG_INSTALL_DIR} ${path_suffix})
        set(export_name "${INSTALL_CMAKE_NAMESPACE}${target}Targets")

        qt_internal_get_min_new_policy_cmake_version(min_new_policy_version)
        qt_internal_get_max_new_policy_cmake_version(max_new_policy_version)
        configure_package_config_file(
            "${QT_CMAKE_DIR}/Qt3rdPartyLibraryConfig.cmake.in"
            "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}Config.cmake"
            INSTALL_DESTINATION "${config_install_dir}"
        )

        write_basic_package_version_file(
            "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}ConfigVersion.cmake"
            VERSION ${PROJECT_VERSION}
            COMPATIBILITY AnyNewerVersion
        )

        qt_install(FILES
            "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}Config.cmake"
            "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}ConfigVersion.cmake"
            DESTINATION "${config_install_dir}"
            COMPONENT Devel
        )

        qt_install(TARGETS ${target}
            EXPORT "${export_name}"
            RUNTIME DESTINATION ${INSTALL_BINDIR}
            LIBRARY DESTINATION ${INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${INSTALL_LIBDIR}
        )

        qt_install(EXPORT ${export_name}
            NAMESPACE "${QT_CMAKE_EXPORT_NAMESPACE}::"
            DESTINATION "${config_install_dir}"
        )

        qt_internal_export_additional_targets_file(
            TARGETS ${target}
            EXPORT_NAME_PREFIX ${INSTALL_CMAKE_NAMESPACE}${target}
            CONFIG_INSTALL_DIR "${config_install_dir}"
        )

        qt_internal_export_modern_cmake_config_targets_file(
            TARGETS ${target}
            EXPORT_NAME_PREFIX ${INSTALL_CMAKE_NAMESPACE}${target}
            CONFIG_INSTALL_DIR "${config_install_dir}"
        )

        set(debug_install_dir "${INSTALL_LIBDIR}")
        if (MINGW)
            set(debug_install_dir "${INSTALL_BINDIR}")
        endif()
        qt_enable_separate_debug_info(${target} "${debug_install_dir}")
        qt_internal_install_pdb_files(${target} "${INSTALL_LIBDIR}")
    endif()
endfunction()

function(qt_install_3rdparty_library_wrap_config_extra_file target)
    if(TARGET "${target}")
        set(use_bundled "ON")
    else()
        set(use_bundled "OFF")
    endif()

    set(QT_USE_BUNDLED_${target} "${use_bundled}" CACHE BOOL "" FORCE)
    set(extra_cmake_code "set(QT_USE_BUNDLED_${target} ${use_bundled} CACHE BOOL \"\" FORCE)")
    configure_file(
        "${QT_CMAKE_DIR}/QtFindWrapConfigExtra.cmake.in"
        "${QT_CONFIG_BUILD_DIR}/${INSTALL_CMAKE_NAMESPACE}/FindWrap${target}ConfigExtra.cmake"
         @ONLY
    )

    qt_install(FILES
        "${QT_CONFIG_BUILD_DIR}/${INSTALL_CMAKE_NAMESPACE}/FindWrap${target}ConfigExtra.cmake"
        DESTINATION "${QT_CONFIG_INSTALL_DIR}/${INSTALL_CMAKE_NAMESPACE}"
        COMPONENT Devel
    )
endfunction()
