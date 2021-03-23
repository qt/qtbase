# Create a QMake list (values space-separated) containing paths.
# Entries that contain whitespace characters are quoted.
function(qt_to_qmake_path_list out_var)
    set(quoted_paths "")
    foreach(path ${ARGN})
        if(path MATCHES "[ \t]")
            list(APPEND quoted_paths "\"${path}\"")
        else()
            list(APPEND quoted_paths "${path}")
        endif()
    endforeach()
    list(JOIN quoted_paths " " result)
    set("${out_var}" "${result}" PARENT_SCOPE)
endfunction()

macro(qt_add_string_to_qconfig_cpp str)
    string(LENGTH "${str}" length)
    string(APPEND QT_CONFIG_STRS "    \"${str}\\0\"\n")
    string(APPEND QT_CONFIG_STR_OFFSETS "    ${QT_CONFIG_STR_OFFSET},\n")
    math(EXPR QT_CONFIG_STR_OFFSET "${QT_CONFIG_STR_OFFSET}+${length}+1")
endmacro()

function(qt_generate_qconfig_cpp)
    set(QT_CONFIG_STR_OFFSET "0")
    set(QT_CONFIG_STR_OFFSETS "")
    set(QT_CONFIG_STRS "")

    # Chop off the "/mkspecs" part of INSTALL_MKSPECSDIR
    get_filename_component(hostdatadir "${INSTALL_MKSPECSDIR}" DIRECTORY)
    if("${hostdatadir}" STREQUAL "")
        set(hostdatadir ".")
    endif()

    # Start first part.
    qt_add_string_to_qconfig_cpp("${INSTALL_DOCDIR}")
    qt_add_string_to_qconfig_cpp("${INSTALL_INCLUDEDIR}")
    qt_add_string_to_qconfig_cpp("${INSTALL_LIBDIR}")
    qt_add_string_to_qconfig_cpp("${INSTALL_LIBEXECDIR}")
    qt_add_string_to_qconfig_cpp("${INSTALL_BINDIR}")
    qt_add_string_to_qconfig_cpp("${INSTALL_PLUGINSDIR}")
    qt_add_string_to_qconfig_cpp("${INSTALL_QMLDIR}")
    qt_add_string_to_qconfig_cpp("${INSTALL_ARCHDATADIR}")
    qt_add_string_to_qconfig_cpp("${INSTALL_DATADIR}")
    qt_add_string_to_qconfig_cpp("${INSTALL_TRANSLATIONSDIR}")
    qt_add_string_to_qconfig_cpp("${INSTALL_EXAMPLESDIR}")
    qt_add_string_to_qconfig_cpp("${INSTALL_TESTSDIR}")

    # Save first part.
    set(QT_CONFIG_STR_OFFSETS_FIRST "${QT_CONFIG_STR_OFFSETS}")
    set(QT_CONFIG_STRS_FIRST "${QT_CONFIG_STRS}")

    # Start second part.
    set(QT_CONFIG_STR_OFFSETS "")
    set(QT_CONFIG_STRS "")

    qt_add_string_to_qconfig_cpp("") # config.input.sysroot
    qt_add_string_to_qconfig_cpp("false") # qmake_sysrootify
    qt_add_string_to_qconfig_cpp("${INSTALL_BINDIR}")
    qt_add_string_to_qconfig_cpp("${INSTALL_LIBEXECDIR}")
    qt_add_string_to_qconfig_cpp("${INSTALL_LIBDIR}")
    qt_add_string_to_qconfig_cpp("${hostdatadir}")
    qt_add_string_to_qconfig_cpp("${QT_QMAKE_TARGET_MKSPEC}")
    qt_add_string_to_qconfig_cpp("${QT_QMAKE_HOST_MKSPEC}")

    # Save second part.
    set(QT_CONFIG_STR_OFFSETS_SECOND "${QT_CONFIG_STR_OFFSETS}")
    set(QT_CONFIG_STRS_SECOND "${QT_CONFIG_STRS}")

    # Settings path / sysconf dir.
    set(QT_SYS_CONF_DIR "${INSTALL_SYSCONFDIR}")

    # Compute and set relocation prefixes.
    # TODO: Clean this up, there's a bunch of unrealistic assumptions here.
    # See qtConfOutput_preparePaths in qtbase/configure.pri.
    if(WIN32)
        set(lib_location_absolute_path
            "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${INSTALL_BINDIR}")
    else()
        set(lib_location_absolute_path
            "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${INSTALL_LIBDIR}")
    endif()
    file(RELATIVE_PATH from_lib_location_to_prefix
         "${lib_location_absolute_path}" "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}")
    set(QT_CONFIGURE_LIBLOCATION_TO_PREFIX_PATH "${from_lib_location_to_prefix}")

    # The QT_CONFIGURE_HOSTBINDIR_TO_*PREFIX_PATH defines are exclusively used by qmake to determine
    # the prefix from the location of the qmake executable. In our build of qmake host_prefix is
    # always the same as ext_prefix, and we can just use CMAKE_INSTALL_PREFIX for the calculation of
    # the relative path between <ext_prefix>/bin and <ext_prefix>.
    set(bin_dir_absolute_path "${CMAKE_INSTALL_PREFIX}/${INSTALL_BINDIR}")
    file(RELATIVE_PATH from_bin_dir_to_prefix "${bin_dir_absolute_path}" "${CMAKE_INSTALL_PREFIX}")
    set(QT_CONFIGURE_HOSTBINDIR_TO_HOSTPREFIX_PATH "${from_bin_dir_to_prefix}")
    set(QT_CONFIGURE_HOSTBINDIR_TO_EXTPREFIX_PATH "${from_bin_dir_to_prefix}")

    # Ensure Windows drive letter is prepended to the install prefix hardcoded
    # into qconfig.cpp, otherwise qmake can't find Qt modules in a static Qt
    # build if there's no qt.conf. Mostly relevant for CI.
    # Given input like
    #    \work/qt/install
    # or
    #    \work\qt\install
    # Expected output is something like
    #   C:/work/qt/install
    # so it includes a drive letter and forward slashes.
    set(QT_CONFIGURE_PREFIX_PATH_STR "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}")
    if(WIN32)
        get_filename_component(
            QT_CONFIGURE_PREFIX_PATH_STR
            "${QT_CONFIGURE_PREFIX_PATH_STR}" REALPATH)
    endif()

    configure_file(global/qconfig.cpp.in global/qconfig.cpp @ONLY)
endfunction()

# In the cross-compiling case, creates a wrapper around the host Qt's
# qmake executable. Also creates a qmake configuration file that sets
# up the host qmake's properties for cross-compiling with this Qt
# build.
function(qt_generate_qmake_wrapper_for_target)
    if(NOT CMAKE_CROSSCOMPILING)
        return()
    endif()

    # Call the configuration file something else but qt.conf to avoid
    # being picked up by the qmake executable that's created if
    # QT_BUILD_TOOLS_WHEN_CROSSCOMPILING is enabled.
    qt_path_join(qt_conf_path "${INSTALL_BINDIR}" "target_qt.conf")

    set(prefix "${CMAKE_INSTALL_PREFIX}")
    set(ext_prefix "${QT_STAGING_PREFIX}")
    set(host_prefix "${QT_HOST_PATH}")
    file(RELATIVE_PATH host_prefix_relative_to_conf_file "${ext_prefix}/${INSTALL_BINDIR}"
        "${host_prefix}")
    file(RELATIVE_PATH ext_prefix_relative_to_conf_file "${ext_prefix}/${INSTALL_BINDIR}"
        "${ext_prefix}")
    file(RELATIVE_PATH ext_prefix_relative_to_host_prefix "${host_prefix}" "${ext_prefix}")

    set(content "")

    # On Android CMAKE_SYSROOT is set, but from qmake's point of view it should not be set, because
    # then qmake generates incorrect Qt module include flags (among other things). Do the same for
    # darwin uikit cross-compilation.
    set(sysroot "")
    if(CMAKE_SYSROOT AND NOT ANDROID AND NOT UIKIT)
        set(sysroot "${CMAKE_SYSROOT}")
    endif()

    # Detect if automatic sysrootification should happen. All of the following must be true:
    # sysroot is set (CMAKE_SYSRROT)
    # prefix is set (CMAKE_INSTALL_PREFIX)
    # extprefix is explicitly NOT set (CMAKE_STAGING_PREFIX, not QT_STAGING_PREFIX because that
    #                                  always ends up having a value)
    if(NOT CMAKE_STAGING_PREFIX AND sysroot)
        set(sysrootify_prefix true)
    else()
        set(sysrootify_prefix false)
        string(APPEND content "[DevicePaths]
Prefix=${prefix}
")
    endif()

    string(APPEND content
        "[Paths]
Prefix=${ext_prefix_relative_to_conf_file}
HostPrefix=${host_prefix_relative_to_conf_file}
HostData=${ext_prefix_relative_to_host_prefix}
Sysroot=${sysroot}
SysrootifyPrefix=${sysrootify_prefix}
TargetSpec=${QT_QMAKE_TARGET_MKSPEC}
HostSpec=${QT_QMAKE_HOST_MKSPEC}
")
    file(GENERATE OUTPUT "${qt_conf_path}" CONTENT "${content}")

    qt_path_join(qmake_wrapper_in_file "${CMAKE_CURRENT_SOURCE_DIR}/bin/qmake-wrapper-for-target")
    set(qmake_wrapper "qmake")
    if(QT_BUILD_TOOLS_WHEN_CROSSCOMPILING)
        # Avoid collisions with the cross-compiled qmake binary.
        string(PREPEND qmake_wrapper "host-")
    endif()
    if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
        string(APPEND qmake_wrapper_in_file ".bat")
        string(APPEND qmake_wrapper ".bat")
    endif()
    string(APPEND qmake_wrapper_in_file ".in")

    set(host_qt_bindir "${host_prefix}/${QT${PROJECT_VERSION_MAJOR}_HOST_INFO_BINDIR}")
    qt_path_join(qmake_wrapper "preliminary" "${qmake_wrapper}")

    configure_file("${qmake_wrapper_in_file}" "${qmake_wrapper}" @ONLY)
    qt_install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${qt_conf_path}"
        DESTINATION "${INSTALL_BINDIR}")
    qt_copy_or_install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/${qmake_wrapper}"
        DESTINATION "${INSTALL_BINDIR}")
endfunction()

# Transforms a CMake Qt module name to a qmake Qt module name.
# Example: Qt6FooPrivate becomes foo_private
function(qt_get_qmake_module_name result module)
    string(REGEX REPLACE "^Qt6" "" module "${module}")
    string(REGEX REPLACE "Private$" "_private" module "${module}")
    string(REGEX REPLACE "Qpa$" "_qpa_lib_private" module "${module}")
    string(TOLOWER "${module}" module)
    set(${result} ${module} PARENT_SCOPE)
endfunction()

function(qt_cmake_build_type_to_qmake_build_config out_var build_type)
    if(build_type STREQUAL "Debug")
        set(cfg debug)
    else()
        set(cfg release)
    endif()
    set(${out_var} ${cfg} PARENT_SCOPE)
endfunction()

function(qt_guess_qmake_build_config out_var)
    if(QT_GENERATOR_IS_MULTI_CONFIG)
        unset(cfg)
        foreach(config_type ${CMAKE_CONFIGURATION_TYPES})
            qt_cmake_build_type_to_qmake_build_config(tmp ${config_type})
            list(APPEND cfg ${tmp})
        endforeach()
        if(cfg)
            list(REMOVE_DUPLICATES cfg)
        else()
            set(cfg debug)
        endif()
    else()
        qt_cmake_build_type_to_qmake_build_config(cfg ${CMAKE_BUILD_TYPE})
    endif()
    set(${out_var} ${cfg} PARENT_SCOPE)
endfunction()

macro(qt_add_qmake_lib_dependency lib dep)
    string(REPLACE "-" "_" dep ${dep})
    string(TOUPPER "${dep}" ucdep)
    list(APPEND QT_QMAKE_LIB_DEPS_${lib} ${ucdep})
endmacro()
