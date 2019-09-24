include(CMakePackageConfigHelpers)

# Install locations:
set(INSTALL_BINDIR "bin" CACHE PATH "Executables [PREFIX/bin]")
set(INSTALL_INCLUDEDIR "include" CACHE PATH "Header files [PREFIX/include]")
set(INSTALL_LIBDIR "lib" CACHE PATH "Libraries [PREFIX/lib]")
set(INSTALL_ARCHDATADIR "." CACHE PATH "Arch-dependent data [PREFIX]")
set(INSTALL_PLUGINSDIR "${INSTALL_ARCHDATADIR}/plugins" CACHE PATH
    "Plugins [ARCHDATADIR/plugins]")

set(INSTALL_TARGETS_DEFAULT_ARGS
    RUNTIME DESTINATION "${INSTALL_BINDIR}"
    LIBRARY DESTINATION "${INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${INSTALL_LIBDIR}" COMPONENT Devel
    INCLUDES DESTINATION "${INSTALL_INCLUDEDIR}"
)

if (WIN32)
    set(_default_libexec "${INSTALL_ARCHDATADIR}/bin")
else()
    set(_default_libexec "${INSTALL_ARCHDATADIR}/libexec")
endif()

set(INSTALL_LIBEXECDIR "${_default_libexec}" CACHE PATH
    "Helper programs [ARCHDATADIR/bin on Windows, ARCHDATADIR/libexec otherwise]")
set(INSTALL_IMPORTDIR "${INSTALL_ARCHDATADIR}/imports" CACHE PATH
    "QML1 imports [ARCHDATADIR/imports]")
set(INSTALL_QMLDIR "${INSTALL_ARCHDATADIR}/qml" CACHE PATH
    "QML2 imports [ARCHDATADIR/qml]")
set(INSTALL_DATADIR "." CACHE PATH  "Arch-independent data [PREFIX]")
set(INSTALL_DOCDIR "${INSTALL_DATADIR}/doc" CACHE PATH "Documentation [DATADIR/doc]")
set(INSTALL_TRANSLATIONSDIR "${INSTALL_DATADIR}/translations" CACHE PATH
    "Translations [DATADIR/translations]")
set(INSTALL_SYSCONFDIR "etc/xdg" CACHE PATH
    "Settings used by Qt programs [PREFIX/etc/xdg]")
set(INSTALL_EXAMPLESDIR "examples" CACHE PATH "Examples [PREFIX/examples]")
set(INSTALL_TESTSDIR "tests" CACHE PATH "Tests [PREFIX/tests]")
set(INSTALL_DESCRIPTIONSDIR "${INSTALL_DATADIR}/modules" CACHE PATH "Module description files directory")

# The variables might have already been set in QtBuildInternalsExtra.cmake if the file is included
# while building a new module and not QtBase. In that case, stop overriding the value.
if(NOT INSTALL_CMAKE_NAMESPACE)
    set(INSTALL_CMAKE_NAMESPACE "Qt${PROJECT_VERSION_MAJOR}"
        CACHE STRING "CMake namespace [Qt${PROJECT_VERSION_MAJOR}]")
endif()
if(NOT QT_CMAKE_EXPORT_NAMESPACE)
    set(QT_CMAKE_EXPORT_NAMESPACE "Qt${PROJECT_VERSION_MAJOR}"
        CACHE STRING "CMake namespace used when exporting targets [Qt${PROJECT_VERSION_MAJOR}]")
endif()

set(QT_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}")

# the default RPATH to be used when installing, but only if it's not a system directory
LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
IF("${isSystemDir}" STREQUAL "-1")
   SET(_default_install_rpath "${CMAKE_INSTALL_PREFIX}/lib")
ENDIF("${isSystemDir}" STREQUAL "-1")

# Default rpath settings: Use rpath for build tree as well as a full path for the installed binaries.
# For origin builds, one needs to override CMAKE_INSTALL_RPATH for example with $ORIGIN/../lib
SET(CMAKE_INSTALL_RPATH "${_default_install_rpath}" CACHE PATH "RPATH for installed binaries")

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Generate a module description file based on the template in ModuleDescription.json.in
function(qt_describe_module target)
    set(path_suffix "${INSTALL_DESCRIPTIONSDIR}")
    qt_path_join(build_dir ${QT_BUILD_DIR} ${path_suffix})
    qt_path_join(install_dir ${QT_INSTALL_DIR} ${path_suffix})

    set(descfile_in "${QT_CMAKE_DIR}/ModuleDescription.json.in")
    set(descfile_out "${build_dir}/${target}.json")
    set(cross_compilation "false")
    if(CMAKE_CROSSCOMPILE)
        set(cross_compilation "true")
    endif()
    configure_file("${descfile_in}" "${descfile_out}")

    qt_install(FILES "${descfile_out}" DESTINATION "${install_dir}")
endfunction()

function(qt_setup_tool_path_command)
    if(NOT WIN32)
        return()
    endif()
    set(bindir "${CMAKE_INSTALL_PREFIX}/${INSTALL_BINDIR}")
    file(TO_NATIVE_PATH "${bindir}" bindir)
    list(APPEND command COMMAND)
    list(APPEND command set PATH=${bindir}$<SEMICOLON>%PATH%)
    set(QT_TOOL_PATH_SETUP_COMMAND "${command}" CACHE INTERNAL "internal command prefix for tool invocations" FORCE)
endfunction()
qt_setup_tool_path_command()

# Platform define path, etc.
set(QT_QMAKE_TARGET_MKSPEC "")
if(WIN32)
    set(QT_DEFAULT_PLATFORM_DEFINITIONS UNICODE _UNICODE WIN32 _ENABLE_EXTENDED_ALIGNED_STORAGE)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        list(APPEND QT_DEFAULT_PLATFORM_DEFINITIONS WIN64 _WIN64)
    endif()
    if(MSVC)
        set(QT_QMAKE_TARGET_MKSPEC win32-msvc)
    elseif(CLANG)
        set(QT_QMAKE_TARGET_MKSPEC win32-clang)
    elseif(MINGW)
        set(QT_QMAKE_TARGET_MKSPEC win32-g++)
        list(APPEND QT_DEFAULT_PLATFORM_DEFINITIONS _WIN32_WINNT=0x0601)
    endif()
elseif(LINUX)
    if(GCC)
        set(QT_QMAKE_TARGET_MKSPEC linux-g++)
    elseif(CLANG)
        set(QT_QMAKE_TARGET_MKSPEC linux-clang)
    elseif(ICC)
        set(QT_QMAKE_TARGET_MKSPEC linux-icc-64)
    endif()
elseif(ANDROID)
    if(GCC)
        set(QT_QMAKE_TARGET_MKSPEC android-g++)
    elseif(CLANG)
        set(QT_QMAKE_TARGET_MKSPEC android-clang)
    endif()
elseif(APPLE)
    set(QT_QMAKE_TARGET_MKSPEC macx-clang)
elseif(EMSCRIPTEN)
    set(QT_QMAKE_TARGET_MKSPEC wasm-emscripten)
endif()

# TODO: Fixme to be correct.
set(QT_QMAKE_HOST_MKSPEC "${QT_QMAKE_TARGET_MKSPEC}")

if(QT_QMAKE_TARGET_MKSPEC)
    set(QT_DEFAULT_PLATFORM_DEFINITION_DIR mkspecs/${QT_QMAKE_TARGET_MKSPEC})
endif()

if(NOT DEFINED QT_DEFAULT_PLATFORM_DEFINITIONS)
    set(QT_DEFAULT_PLATFORM_DEFINITIONS "")
endif()

set(QT_PLATFORM_DEFINITIONS ${QT_DEFAULT_PLATFORM_DEFINITIONS}
    CACHE STRING "Qt platform specific pre-processor defines")
set(QT_PLATFORM_DEFINITION_DIR ${QT_DEFAULT_PLATFORM_DEFINITION_DIR}
    CACHE PATH "Path to directory that contains qplatformdefs.h")
set(QT_NAMESPACE "" CACHE STRING "Qt Namespace")
if(QT_NAMESPACE STREQUAL "")
    set(QT_HAS_NAMESPACE OFF)
else()
    set(QT_HAS_NAMESPACE ON)
endif()


function(qt_internal_clear_qt_repo_known_modules)
    set(QT_REPO_KNOWN_MODULES "" CACHE INTERNAL "Known current repo Qt modules" FORCE)
endfunction()

function(qt_internal_add_qt_repo_known_module)
    set(QT_REPO_KNOWN_MODULES ${QT_REPO_KNOWN_MODULES} ${ARGN}
        CACHE INTERNAL "Known current repo Qt modules" FORCE)
endfunction()

function(qt_internal_get_qt_repo_known_modules out_var)
    set("${out_var}" "${QT_REPO_KNOWN_MODULES}" PARENT_SCOPE)
endfunction()

# Gets the list of all known Qt modules both found and that were built as part of the
# current project.
function(qt_internal_get_qt_all_known_modules out_var)
    qt_internal_get_qt_repo_known_modules(repo_known_modules)
    set(known_modules ${QT_ALL_MODULES_FOUND_VIA_FIND_PACKAGE} ${repo_known_modules})
    list(REMOVE_DUPLICATES known_modules)
    set("${out_var}" "${known_modules}" PARENT_SCOPE)
endfunction()

macro(qt_internal_set_qt_known_plugins)
    set(QT_KNOWN_PLUGINS ${ARGN} CACHE INTERNAL "Known Qt plugins" FORCE)
endmacro()

### Global plug-in types handling ###
# QT_REPO_KNOWN_PLUGIN_TYPES and QT_ALL_PLUGIN_TYPES_FOUND_VIA_FIND_PACKAGE
# hold a list of plug-in types (e.G. "imageformats;bearer")
function(qt_internal_clear_qt_repo_known_plugin_types)
    set(QT_REPO_KNOWN_PLUGIN_TYPES "" CACHE INTERNAL "Known current repo Qt plug-in types" FORCE)
endfunction()

function(qt_internal_add_qt_repo_known_plugin_types)
    set(QT_REPO_KNOWN_PLUGIN_TYPES ${QT_REPO_KNOWN_PLUGIN_TYPES} ${ARGN}
        CACHE INTERNAL "Known current repo Qt plug-in types" FORCE)
endfunction()

function(qt_internal_get_qt_repo_known_plugin_types out_var)
    set("${out_var}" "${QT_REPO_KNOWN_PLUGIN_TYPES}" PARENT_SCOPE)
endfunction()

function(qt_internal_get_qt_all_known_plugin_types out_var)
    qt_internal_get_qt_repo_known_plugin_types(repo_known_plugin_types)
    set(known ${QT_ALL_PLUGIN_TYPES_FOUND_VIA_FIND_PACKAGE} ${repo_known_plugin_types})
    list(REMOVE_DUPLICATES known)
    set("${out_var}" "${known}" PARENT_SCOPE)
endfunction()

# Reset:
qt_internal_clear_qt_repo_known_modules()
qt_internal_clear_qt_repo_known_plugin_types()
qt_internal_set_qt_known_plugins("")

set(QT_KNOWN_MODULES_WITH_TOOLS "" CACHE INTERNAL "Known Qt modules with tools" FORCE)
macro(qt_internal_append_known_modules_with_tools module)
    if(NOT ${module} IN_LIST QT_KNOWN_MODULES_WITH_TOOLS)
        set(QT_KNOWN_MODULES_WITH_TOOLS "${QT_KNOWN_MODULES_WITH_TOOLS};${module}"
            CACHE INTERNAL "Known Qt modules with tools" FORCE)
    endif()
endmacro()

macro(qt_internal_append_known_module_tool module tool)
    if(NOT ${tool} IN_LIST QT_KNOWN_MODULE_${module}_TOOLS)
        list(APPEND QT_KNOWN_MODULE_${module}_TOOLS "${tool}")
        set(QT_KNOWN_MODULE_${module}_TOOLS "${QT_KNOWN_MODULE_${module}_TOOLS}"
            CACHE INTERNAL "Known Qt module ${module} tools" FORCE)
    endif()
endmacro()

# Reset syncqt cache variable, to make sure it gets recomputed on reconfiguration, otherwise
# it might not get installed.
unset(QT_SYNCQT CACHE)

# For adjusting variables when running tests, we need to know what
# the correct variable is for separating entries in PATH-alike
# variables.
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(QT_PATH_SEPARATOR "\\;")
else()
    set(QT_PATH_SEPARATOR ":")
endif()

# Compute the values of QT_BUILD_DIR, QT_INSTALL_DIR, QT_CONFIG_BUILD_DIR, QT_CONFIG_INSTALL_DIR
# taking into account whether the current build is a prefix build or a non-prefix build.
#
# These values should be prepended to file paths in commands or properties,
# in order to correctly place generated Config files, generated Targets files,
# excutables / libraries, when copying / installing files, etc.
#
# The build dir variables will always be absolute paths.
# The QT_INSTALL_DIR variable will have a relative path in a prefix build,
# which means that it can be empty, so use qt_join_path to prevent accidental absolute paths.
if(QT_WILL_INSTALL)
    # In the usual prefix build case, the build dir is the current module build dir,
    # and the install dir is the prefix, so we don't set it.
    set(QT_BUILD_DIR "${CMAKE_BINARY_DIR}")
    set(QT_INSTALL_DIR "")
else()
    # When doing a non-prefix build, both the build dir and install dir are the same,
    # pointing to the qtbase build dir.
    set(QT_BUILD_DIR "${CMAKE_INSTALL_PREFIX}")
    set(QT_INSTALL_DIR "${QT_BUILD_DIR}")
endif()
set(__config_path_part "${INSTALL_LIBDIR}/cmake")
set(QT_CONFIG_BUILD_DIR "${QT_BUILD_DIR}/${__config_path_part}")
set(QT_CONFIG_INSTALL_DIR "${QT_INSTALL_DIR}")
if(QT_CONFIG_INSTALL_DIR)
    string(APPEND QT_CONFIG_INSTALL_DIR "/")
endif()
string(APPEND QT_CONFIG_INSTALL_DIR ${__config_path_part})
unset(__config_path_part)

# This is used to hold extra cmake code that should be put into QtBuildInternalsExtra.cmake file
# at the QtPostProcess stage.
set(QT_BUILD_INTERNALS_EXTRA_CMAKE_CODE "")

# Save the value of the current first project source dir.
# This will be /path/to/qtbase for qtbase both in a super-build and a non super-build.
# This will be /path/to/qtbase/tests when building standalone tests.
set(QT_TOP_LEVEL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

# Prevent warnings about object files without any symbols. This is a common
# thing in Qt as we tend to build files unconditionally, and then use ifdefs
# to compile out parts that are not relevant.
if(CMAKE_HOST_APPLE AND APPLE)
    foreach(lang ASM C CXX)
        # We have to tell 'ar' to not run ranlib by itself, by passing the 'S' option
        set(CMAKE_${lang}_ARCHIVE_CREATE "<CMAKE_AR> qcS <TARGET> <LINK_FLAGS> <OBJECTS>")
        set(CMAKE_${lang}_ARCHIVE_APPEND "<CMAKE_AR> qS <TARGET> <LINK_FLAGS> <OBJECTS>")
        set(CMAKE_${lang}_ARCHIVE_FINISH "<CMAKE_RANLIB> -no_warning_for_no_symbols <TARGET>")
    endforeach()
endif()

# Functions and macros:

# qt_remove_args can remove arguments from an existing list of function
# arguments in order to pass a filtered list of arguments to a different function.
# Parameters:
#   out_var: result of remove all arguments specified by ARGS_TO_REMOVE from ALL_ARGS
#   ARGS_TO_REMOVE: Arguments to remove.
#   ALL_ARGS: All arguments supplied to cmake_parse_arguments or
#   qt_parse_all_arguments
#   from which ARGS_TO_REMOVE should be removed from. We require all the
#   arguments or we can't properly identify the range of the arguments detailed
#   in ARGS_TO_REMOVE.
#   ARGS: Arguments passed into the function, usually ${ARGV}
#
#   E.g.:
#   We want to forward all arguments from foo to bar, execpt ZZZ since it will
#   trigger an error in bar.
#
#   foo(target BAR .... ZZZ .... WWW ...)
#   bar(target BAR.... WWW...)
#
#   function(foo target)
#       qt_parse_all_arguments(arg "" "" "BAR;ZZZ;WWW ${ARGV})
#       qt_remove_args(forward_args
#           ARGS_TO_REMOVE ${target} ZZZ
#           ALL_ARGS ${target} BAR ZZZ WWW
#           ARGS ${ARGV}
#       )
#       bar(${target} ${forward_args})
#   endfunction()
#
function(qt_remove_args out_var)
    cmake_parse_arguments(arg "" "" "ARGS_TO_REMOVE;ALL_ARGS;ARGS" ${ARGN})
    set(result ${arg_ARGS})
    foreach(arg IN LISTS arg_ARGS_TO_REMOVE)
        # find arg
        list(FIND result ${arg} find_result)
        if (NOT find_result EQUAL -1)
            # remove arg
            list(REMOVE_AT result ${find_result})
            list(LENGTH result result_len)
            list(GET result ${find_result} arg_current)
            # remove values until we hit another arg
            while(NOT ${arg_current} IN_LIST arg_ALL_ARGS AND find_result LESS result_len)
                list(REMOVE_AT result ${find_result})
                list(GET result ${find_result} arg_current)
                list(LENGTH result result_len)
            endwhile()
        endif()
    endforeach()
    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()
# Wraps install() command. In a prefix build, simply passes along arguments to install().
# In a non-prefix build, handles association of targets to export names, and also calls export().
function(qt_install)
    set(flags)
    set(options EXPORT DESTINATION NAMESPACE)
    set(multiopts TARGETS)
    cmake_parse_arguments(arg "${flags}" "${options}" "${multiopts}" ${ARGN})

    if(arg_TARGETS)
        set(is_install_targets TRUE)
    endif()

    # In a prefix build, always invoke install() without modification.
    # In a non-prefix build, pass install(TARGETS) commands to allow
    # association of targets to export names, so we can later use the export names
    # in export() commands.
    if(QT_WILL_INSTALL OR is_install_targets)
        install(${ARGV})
    endif()

    # Exit early if this is a prefix build.
    if(QT_WILL_INSTALL)
        return()
    endif()

    # In a non-prefix build, when install(EXPORT) is called,
    # also call export(EXPORT) to generate build tree target files.
    if(NOT is_install_targets AND arg_EXPORT)
        set(namespace_option "")
        if(arg_NAMESPACE)
            set(namespace_option NAMESPACE ${arg_NAMESPACE})
        endif()
        export(EXPORT ${arg_EXPORT}
               ${namespace_option}
               FILE "${arg_DESTINATION}/${arg_EXPORT}.cmake")
    endif()
endfunction()

# Copies files using file(COPY) signature in non-prefix builds.
function(qt_non_prefix_copy)
    if(NOT QT_WILL_INSTALL)
        file(${ARGV})
    endif()
endfunction()

# Use case is installing files in a prefix build, or copying them to the correct build dir
# in a non-prefix build.
# Pass along arguments as you would pass them to install().
# Only supports FILES, PROGRAMS and DIRECTORY signature, and without fancy things
# like OPTIONAL or RENAME or COMPONENT.
function(qt_copy_or_install)
    set(flags FILES PROGRAMS DIRECTORY)
    set(options)
    set(multiopts)
    cmake_parse_arguments(arg "${flags}" "${options}" "${multiopts}" ${ARGN})

    # Remember which option has to be passed to the install command.
    set(argv_copy ${ARGV})
    if(arg_FILES)
        set(install_option "FILES")
    elseif(arg_PROGRAMS)
        set(install_option "PROGRAMS")
    elseif(arg_DIRECTORY)
        set(install_option "DIRECTORY")
    endif()

    list(REMOVE_AT argv_copy 0)
    qt_install(${install_option} ${argv_copy})
    qt_non_prefix_copy(COPY ${argv_copy})
endfunction()

# Hacky way to remove the install target in non-prefix builds.
# We need to associate targets with export names, and that is only possible to do with the
# install(TARGETS) command. But in a non-prefix build, we don't want to install anything.
# To make sure that developers don't accidentally run make install, replace the generated
# cmake_install.cmake file with an empty file. To do this, always create a new temporary file
# at CMake configuration step, and use it as an input to a custom command that replaces the
# cmake_install.cmake file with an empty one. This means we will always replace the file on
# every reconfiguration, but not when doing null builds.
function(remove_install_target)
    set(file_in "${CMAKE_BINARY_DIR}/.remove_cmake_install_in.txt")
    set(file_generated "${CMAKE_BINARY_DIR}/.remove_cmake_install_generated.txt")
    set(cmake_install_file "${CMAKE_BINARY_DIR}/cmake_install.cmake")
    file(WRITE ${file_in} "")

    add_custom_command(OUTPUT ${file_generated}
        COMMAND ${CMAKE_COMMAND} -E copy ${file_in} ${file_generated}
        COMMAND ${CMAKE_COMMAND} -E remove ${cmake_install_file}
        COMMAND ${CMAKE_COMMAND} -E touch ${cmake_install_file}
        COMMENT "Removing cmake_install.cmake"
        MAIN_DEPENDENCY ${file_in})

    add_custom_target(remove_cmake_install ALL DEPENDS ${file_generated})
endfunction()

function(qt_set_up_nonprefix_build)
    if(NOT QT_WILL_INSTALL)
        remove_install_target()
    endif()
endfunction()

function(qt_is_imported_target target out_var)
    if(NOT TARGET "${target}")
        set(target "${QT_CMAKE_EXPORT_NAMESPACE}::${target}")
    endif()
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "Invalid target given to qt_is_imported_target: ${target}")
    endif()
    get_target_property(is_imported "${target}" IMPORTED)
    set(${out_var} "${is_imported}" PARENT_SCOPE)
endfunction()

# Generates module .pri files for consumption by qmake
function(qt_generate_module_pri_file target target_path pri_files_var)
    set(flags INTERNAL_MODULE HEADER_MODULE)
    set(options)
    set(multiopts QMAKE_MODULE_CONFIG)
    cmake_parse_arguments(arg "${flags}" "${options}" "${multiopts}" ${ARGN})

    qt_internal_module_info(module "${target}")
    set(pri_files)

    set(property_prefix)
    if(arg_HEADER_MODULE)
        set(property_prefix "interface_")
    endif()

    get_target_property(enabled_features "${target}"
                        "${property_prefix}QT_ENABLED_PUBLIC_FEATURES")
    get_target_property(disabled_features "${target}"
                        "${property_prefix}QT_DISABLED_PUBLIC_FEATURES")
    get_target_property(enabled_private_features "${target}"
                        "${property_prefix}QT_ENABLED_PRIVATE_FEATURES")
    get_target_property(disabled_private_features "${target}"
                        "${property_prefix}QT_DISABLED_PRIVATE_FEATURES")

    foreach(var enabled_features disabled_features enabled_private_features disabled_private_features)
        if(${var} STREQUAL "${var}-NOTFOUND")
            set(${var} "")
        else()
            string (REPLACE ";" " " ${var} "${${var}}")
        endif()
    endforeach()

    if(arg_QMAKE_MODULE_CONFIG)
        string(REPLACE ";" " " module_config "${arg_QMAKE_MODULE_CONFIG}")
        set(module_config "\nQT.${module_lower}.CONFIG = ${module_config}")
    else()
        set(module_config "")
    endif()

    if (NOT ${arg_INTERNAL_MODULE})
        qt_path_join(pri_file_name "${target_path}" "qt_lib_${module_lower}.pri")
        list(APPEND pri_files "${pri_file_name}")

        file(GENERATE
            OUTPUT "${pri_file_name}"
            CONTENT
        "QT.${module_lower}.VERSION = ${PROJECT_VERSION}
QT.${module_lower}.name = ${module}
QT.${module_lower}.module = ${module_versioned}
QT.${module_lower}.libs = $$QT_MODULE_LIB_BASE
QT.${module_lower}.includes = $$QT_MODULE_INCLUDE_BASE $$QT_MODULE_INCLUDE_BASE/${module}
QT.${module_lower}.frameworks =
QT.${module_lower}.bins = $$QT_MODULE_BIN_BASE
QT.${module_lower}.depends =
QT.${module_lower}.uses =
QT.${module_lower}.module_config = v2
QT.${module_lower}.DEFINES = QT_${module_define}_LIB
QT.${module_lower}.enabled_features = ${enabled_features}
QT.${module_lower}.disabled_features = ${disabled_features}${module_config}
QT_MODULES += ${module_lower}
"
        )
    endif()

    qt_path_join(private_pri_file "${target_path}" "qt_lib_${module_lower}_private.pri")
    list(APPEND pri_files "${private_pri_file}")

    file(GENERATE
        OUTPUT "${private_pri_file}"
        CONTENT
        "QT.${module_lower}_private.VERSION = ${PROJECT_VERSION}
QT.${module_lower}_private.name = ${module}
QT.${module_lower}_private.module =
QT.${module_lower}_private.libs = $$QT_MODULE_LIB_BASE
QT.${module_lower}_private.includes = $$QT_MODULE_INCLUDE_BASE/${module}/${PROJECT_VERSION} $$QT_MODULE_INCLUDE_BASE/${module}/${PROJECT_VERSION}/${module}
QT.${module_lower}_private.frameworks =
QT.${module_lower}_private.depends = ${module_lower}
QT.${module_lower}_private.uses =
QT.${module_lower}_private.module_config = v2
QT.${module_lower}_private.enabled_features = ${enabled_private_features}
QT.${module_lower}_private.disabled_features = ${disabled_private_features}
"
    )

    set("${pri_files_var}" "${pri_files}" PARENT_SCOPE)
endfunction()

function(qt_generate_global_config_pri_file)
    qt_path_join(qconfig_pri_target_path ${PROJECT_BINARY_DIR} mkspecs)
    qt_path_join(qconfig_pri_target_path "${qconfig_pri_target_path}" "qconfig.pri")

    get_target_property(enabled_features GlobalConfig INTERFACE_QT_ENABLED_PUBLIC_FEATURES)
    get_target_property(disabled_features GlobalConfig INTERFACE_QT_DISABLED_PUBLIC_FEATURES)

    # configure2cmake skips the "static" feature, so emulate it here for qmake support:
    if(QT_BUILD_SHARED_LIBS OR BUILD_SHARED_LIBS)
        list(APPEND enabled_features shared)
        list(APPEND disabled_features static)
        set(qt_build_config_type "shared")
    else()
        list(APPEND enabled_features static)
        list(APPEND disabled_features shared)
        set(qt_build_config_type "static")
    endif()

    # configure2cmake skips the "rpath" feature, so emulate it here for qmake support:
    if(CMAKE_SKIP_RPATH)
        list(APPEND disabled_features rpath)
    elseif(LINUX OR APPLE)
        list(APPEND enabled_features rpath)
    endif()

    string (REPLACE ";" " " enabled_features "${enabled_features}")
    string (REPLACE ";" " " disabled_features "${disabled_features}")

    # Add some required CONFIG entries.
    set(config_entries "")
    if(CMAKE_BUILD_TYPE STREQUAL Debug)
        list(APPEND config_entries "debug")
    elseif(CMAKE_BUILD_TYPE STREQUAL Release)
        list(APPEND config_entries "release")
    endif()
    list(APPEND config_entries "${qt_build_config_type}")
    string (REPLACE ";" " " config_entries "${config_entries}")

    file(GENERATE
        OUTPUT "${qconfig_pri_target_path}"
        CONTENT
        "QT_ARCH = ${TEST_architecture_arch}
QT_BUILDABI = ${TEST_buildAbi}
QT.global.enabled_features = ${enabled_features}
QT.global.disabled_features = ${disabled_features}
QT_VERSION = ${PROJECT_VERSION}
QT_MAJOR_VERSION = ${PROJECT_VERSION_MAJOR}
QT_MINOR_VERSION = ${PROJECT_VERSION_MINOR}
QT_PATCH_VERSION = ${PROJECT_VERSION_PATCH}
CONFIG -= link_prl # we do not create prl files right now
CONFIG += ${config_entries}
"
    )
    qt_install(FILES "${qconfig_pri_target_path}" DESTINATION mkspecs)
endfunction()

# Takes a list of path components and joins them into one path separated by forward slashes "/",
# and saves the path in out_var.
function(qt_path_join out_var)
    # Remove output variable.
    set(argv ${ARGV})
    list(REMOVE_AT argv 0)

    # Join the path components.
    string(JOIN "/" path ${argv})
    set(${out_var} ${path} PARENT_SCOPE)
endfunction()

function(qt_internal_export_modern_cmake_config_targets_file)
    cmake_parse_arguments(__arg "" "EXPORT_NAME_PREFIX;CONFIG_INSTALL_DIR" "TARGETS" ${ARGN})

    set(export_name "${__arg_EXPORT_NAME_PREFIX}VersionlessTargets")
    foreach(target ${__arg_TARGETS})
        if (TARGET "${target}Versionless")
            continue()
        endif()

        add_library("${target}Versionless" INTERFACE)
        target_link_libraries("${target}Versionless" INTERFACE "${target}")
        set_target_properties("${target}Versionless" PROPERTIES EXPORT_NAME "${target}")
        qt_install(TARGETS "${target}Versionless" EXPORT ${export_name})
    endforeach()
    qt_install(EXPORT ${export_name} NAMESPACE Qt:: DESTINATION "${__arg_CONFIG_INSTALL_DIR}")
endfunction()

# Print all variables defined in the current scope.
macro(qt_debug_print_variables)
    cmake_parse_arguments(__arg "DEDUP" "" "MATCH;IGNORE" ${ARGN})
    message("Known Variables:")
    get_cmake_property(__variableNames VARIABLES)
    list (SORT __variableNames)
    if (__arg_DEDUP)
        list(REMOVE_DUPLICATES __variableNames)
    endif()

    foreach(__var ${__variableNames})
        set(__ignore OFF)
        foreach(__i ${__arg_IGNORE})
            if(__var MATCHES "${__i}")
                set(__ignore ON)
                break()
            endif()
        endforeach()

        if (__ignore)
            continue()
        endif()

        set(__show OFF)
        foreach(__i ${__arg_MATCH})
            if(__var MATCHES "${__i}")
                set(__show ON)
                break()
            endif()
        endforeach()

        if (__show)
            message("    ${__var}=${${__var}}.")
        endif()
    endforeach()
endmacro()


macro(assert)
    if (${ARGN})
    else()
        message(FATAL_ERROR "ASSERT: ${ARGN}.")
    endif()
endmacro()


function(qt_create_nolink_target target dependee_target)
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "${target} does not exist when trying to build a nolink target.")
    endif()
    get_target_property(type "${target}" TYPE)
    if(type STREQUAL EXECUTABLE)
        message(FATAL_ERROR "${target} must be a library of some kind.")
    endif()
    if(type STREQUAL OBJECT_LIBRARY)
        message(FATAL_ERROR "${target} must not be an object library.")
    endif()

    # Strip off the namespace prefix, so from Vulkan::Vulkan to Vulkan, and then append _nolink.
    string(REGEX REPLACE "^.*::" "" non_prefixed_target ${target})
    set(nolink_target "${non_prefixed_target}_nolink")

    # Create the nolink interface target, assign the properties from the original target,
    # associate the nolink target with the same export which contains
    # the target that uses the _nolink target.
    # Also create a namespaced alias of the form {$target}::${target}_nolink which is used by
    # our modules.
    if(NOT TARGET "${nolink_target}")
        add_library("${nolink_target}" INTERFACE)
        set(prefixed_nolink_target "${target}_nolink")
        set_target_properties("${nolink_target}" PROPERTIES
                              INTERFACE_INCLUDE_DIRECTORIES
                              $<TARGET_PROPERTY:${target},INTERFACE_INCLUDE_DIRECTORIES>
                              INTERFACE_SYSTEM_INCLUDE_DIRECTORIES
                              $<TARGET_PROPERTY:${target},INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
                              INTERFACE_COMPILE_DEFINITIONS
                              $<TARGET_PROPERTY:${target},INTERFACE_COMPILE_DEFINITIONS>
                              INTERFACE_COMPILE_OPTIONS
                              $<TARGET_PROPERTY:${target},INTERFACE_COMPILE_OPTIONS>
                              INTERFACE_COMPILE_FEATURES
                              $<TARGET_PROPERTY:${target},INTERFACE_COMPILE_FEATURES>)

        add_library(${prefixed_nolink_target} ALIAS ${nolink_target})

        set(export_name "${INSTALL_CMAKE_NAMESPACE}${dependee_target}Targets")
        qt_install(TARGETS ${nolink_target} EXPORT ${export_name})
    endif()
endfunction()

function(qt_ensure_perl)
    if(DEFINED HOST_PERL)
        return()
    endif()
    find_program(HOST_PERL "perl" DOC "Perl binary")
    if (NOT HOST_PERL)
        message(FATAL_ERROR "Perl needs to be available to build Qt.")
    endif()
endfunction()


function(qt_ensure_sync_qt)
    qt_ensure_perl()
    if(DEFINED QT_SYNCQT)
        return()
    endif()

    # When building qtbase, use the source syncqt, otherwise use the installed one.
    set(SYNCQT_FROM_SOURCE "${QtBase_SOURCE_DIR}/bin/syncqt.pl")
    if(NOT ("${QtBase_SOURCE_DIR}" STREQUAL "") AND EXISTS "${SYNCQT_FROM_SOURCE}")
        set(QT_SYNCQT "${SYNCQT_FROM_SOURCE}" CACHE FILEPATH "syncqt script")
        message(STATUS "Using source syncqt found at: ${QT_SYNCQT}")

        qt_path_join(syncqt_install_dir ${QT_INSTALL_DIR} ${INSTALL_LIBEXECDIR})
        qt_copy_or_install(PROGRAMS "${SYNCQT_FROM_SOURCE}"
                           DESTINATION "${syncqt_install_dir}")
    else()
        get_filename_component(syncqt_absolute_path
                               "${CMAKE_INSTALL_PREFIX}/${INSTALL_LIBEXECDIR}/syncqt.pl"
                               ABSOLUTE)
        set(QT_SYNCQT "${syncqt_absolute_path}" CACHE FILEPATH "syncqt script")
        message(STATUS "Using installed syncqt found at: ${QT_SYNCQT}")
    endif()
endfunction()

# A version of cmake_parse_arguments that makes sure all arguments are processed and errors out
# with a message about ${type} having received unknown arguments.
macro(qt_parse_all_arguments result type flags options multiopts)
    cmake_parse_arguments(${result} "${flags}" "${options}" "${multiopts}" ${ARGN})
    if(DEFINED ${result}_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown arguments were passed to ${type} (${${result}_UNPARSED_ARGUMENTS}).")
    endif()
endmacro()


function(qt_internal_add_link_flags_no_undefined target)
    if (NOT QT_BUILD_SHARED_LIBS)
        return()
    endif()
    if (GCC OR CLANG)
        if(APPLE)
            set(no_undefined_flag "-Wl,-undefined,error")
        elseif(LINUX OR MINGW OR ANDROID)
            set(no_undefined_flag "-Wl,--no-undefined")
        else()
            message(FATAL_ERROR "Platform linker doesn't support erroring upon encountering undefined symbols. Target:\"${target}\".")
        endif()
        target_link_options("${target}" PRIVATE "${no_undefined_flag}")
    endif()
endfunction()

function(qt_internal_add_link_flags_gc_sections target visibility)
    set(possible_visibilities PRIVATE INTERFACE PUBLIC)
    list(FIND possible_visibilities "${visibility}" known_visibility)
    if (known_visibility EQUAL "-1")
        message(FATAL_ERROR "Visibitily setting must be one of PRIVATE, INTERFACE or PUBLIC.")
    endif()

    if ((GCC OR CLANG) AND NOT EMSCRIPTEN)
        if(APPLE)
            set(gc_sections_flag "-Wl,-dead_strip")
        elseif(LINUX OR BSD OR SOLARIS OR WIN32 OR ANDROID)
            set(gc_sections_flag "-Wl,--gc-sections")
        else()
            message(FATAL_ERROR "Platform linker doesn't support gc sections. Target:\"${target}\".")
        endif()
        target_link_options("${target}" ${visibility} "${gc_sections_flag}")
    endif()
endfunction()

function(qt_internal_add_linker_version_script target)
    qt_parse_all_arguments(arg "qt_internal_add_linker" "INTERNAL" "" "PRIVATE_HEADERS" ${ARGN})

    if (TEST_ld_version_script)
        if (arg_INTERNAL)
            set(contents "Qt_${PROJECT_VERSION_MAJOR}_PRIVATE_API { *; };")
        else()
            set(contents "Qt_${PROJECT_VERSION_MAJOR}_PRIVATE_API {\n    qt_private_api_tag*;\n")
            foreach(ph ${arg_PRIVATE_HEADERS})
                string(APPEND contents "    @FILE:${ph}@\n")
            endforeach()
            string(APPEND contents "};\n")
            set(current "Qt_${PROJECT_VERSION_MAJOR}")
            if (QT_NAMESPACE STREQUAL "")
                set(tag_symbol "qt_version_tag")
            else()
                set(tag_symbol "qt_version_tag_${QT_NAMESPACE}")
            endif()
            string(APPEND contents "${current} { *; };\n")

            foreach(minor_version RANGE ${PROJECT_VERSION_MINOR})
                set(previous "${current}")
                set(current "Qt_${PROJECT_VERSION_MAJOR}.${minor_version}")
                if (minor_version EQUAL ${PROJECT_VERSION_MINOR})
                   string(APPEND contents "${current} { ${tag_symbol}; } ${previous};\n")
                else()
                   string(APPEND contents "${current} {} ${previous};\n")
                endif()
            endforeach()

            set(infile "${CMAKE_CURRENT_BINARY_DIR}/${target}.version.in")
            set(outfile "${CMAKE_CURRENT_BINARY_DIR}/${target}.version")

            file(GENERATE OUTPUT "${infile}" CONTENT "${contents}")

            qt_ensure_perl()

            add_custom_command(TARGET "${target}" PRE_LINK
                COMMAND "${HOST_PERL}" "${PROJECT_SOURCE_DIR}/mkspecs/features/data/unix/findclasslist.pl" < "${infile}" > "${outfile}"
                BYPRODUCTS "${outfile}" DEPENDS "${infile}"
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                COMMENT "Generating version linker script"
            )
            target_link_options("${target}" PRIVATE "-Wl,--version-script,${outfile}")
        endif()
    endif()
endfunction()


# Get a set of Qt module related values based on the target name.
# When doing qt_internal_module_info(foo Core) this method will set
# the following variables in the caller's scope:
#  * foo with the value "QtCore"
#  * foo_versioned with the value "Qt6Core" (based on major Qt version)
#  * foo_upper with the value "CORE"
#  * foo_lower with the value "core"
#  * foo_repo_include_dir with the module's include directory
#    e.g for QtQuick it would be qtdeclarative_build_dir/include for a prefix build or
#                                qtbase_build_dir/include for a non-prefix build
#  * foo_include_dir with the module's include directory
#    e.g for QtQuick it would be qtdeclarative_build_dir/include/QtQuick for a prefix build or
#                                qtbase_build_dir/include/QtQuick for a non-prefix build
#  * foo_define same as foo_uper but with - replaced as _
function(qt_internal_module_info result target)
    set(module "Qt${target}")
    set("${result}" "${module}" PARENT_SCOPE)
    set("${result}_versioned" "Qt${PROJECT_VERSION_MAJOR}${target}" PARENT_SCOPE)
    string(TOUPPER "${target}" upper)
    string(TOLOWER "${target}" lower)#  * foo_upper with the value "CORE"
    string(REPLACE "-" "_" define "${upper}")
    string(REPLACE "." "_" define "${define}")
    set("${result}_upper" "${upper}" PARENT_SCOPE)
    set("${result}_lower" "${lower}" PARENT_SCOPE)
    set("${result}_repo_include_dir" "${QT_BUILD_DIR}/include" PARENT_SCOPE)
    set("${result}_include_dir" "${QT_BUILD_DIR}/include/${module}" PARENT_SCOPE)
    set("${result}_define" "${define}" PARENT_SCOPE)
endfunction()


set(__default_private_args "SOURCES;LIBRARIES;INCLUDE_DIRECTORIES;DEFINES;DBUS_ADAPTOR_BASENAME;DBUS_ADAPTOR_FLAGS;DBUS_ADAPTOR_SOURCES;DBUS_INTERFACE_BASENAME;DBUS_INTERFACE_FLAGS;DBUS_INTERFACE_SOURCES;FEATURE_DEPENDENCIES;COMPILE_OPTIONS;LINK_OPTIONS;MOC_OPTIONS;DISABLE_AUTOGEN_TOOLS;ENABLE_AUTOGEN_TOOLS;PLUGIN_TYPES")

set(__default_public_args "PUBLIC_LIBRARIES;PUBLIC_INCLUDE_DIRECTORIES;PUBLIC_DEFINES;PUBLIC_COMPILE_OPTIONS;PUBLIC_LINK_OPTIONS")


option(QT_CMAKE_DEBUG_EXTEND_TARGET "Debug extend_target calls in Qt's build system" OFF)

# Initial autogen setup for a target to specify certain CMake properties which are common
# to all autogen tools. Also enable AUTOMOC by default.
function(qt_autogen_tools_initial_setup target)
    set_property(TARGET "${target}" PROPERTY INTERFACE_QT_MAJOR_VERSION ${PROJECT_VERSION_MAJOR})
    set_property(TARGET "${target}" APPEND PROPERTY COMPATIBLE_INTERFACE_STRING QT_MAJOR_VERSION)

    set_directory_properties(PROPERTIES
                             QT_VERSION_MAJOR ${PROJECT_VERSION_MAJOR}
                             QT_VERSION_MINOR ${PROJECT_VERSION_MINOR})

    qt_enable_autogen_tool(${target} "moc" ON)
endfunction()

# Enables or disables an autogen tool like moc, uic or rcc on ${target}.
function(qt_enable_autogen_tool target tool enable)
    string(TOUPPER "${tool}" captitalAutogenTool)

    get_target_property(tool_enabled ${target} AUTO${captitalAutogenTool})
    get_target_property(autogen_target_depends ${target} AUTOGEN_TARGET_DEPENDS)

    if(NOT autogen_target_depends)
        set(autogen_target_depends "")
    endif()
    set(tool_executable "$<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::${tool}>")
    set(tool_target_name ${QT_CMAKE_EXPORT_NAMESPACE}::${tool})

    if(enable)
        list(APPEND autogen_target_depends ${tool_target_name})
    else()
        list(REMOVE_ITEM autogen_target_depends ${tool_target_name})
    endif()

    # f66c1db16c050c9d685a44a38ad7c5cf9f6fcc96 in qtbase introduced a new macro
    # that the moc scanner has to look for. Inform the CMake moc scanner about it.
    if(tool STREQUAL "moc" AND enable)
        set_target_properties("${target}" PROPERTIES
            AUTOMOC_MACRO_NAMES "Q_OBJECT;Q_GADGET;Q_NAMESPACE;Q_NAMESPACE_EXPORT;Q_ENUM_NS")
    endif()

    set_target_properties("${target}"
                          PROPERTIES
                          AUTO${captitalAutogenTool} "${enable}"
                          AUTO${captitalAutogenTool}_EXECUTABLE "${tool_executable}"
                          AUTOGEN_TARGET_DEPENDS "${autogen_target_depends}"
                          )
endfunction()

# This function adds or removes additional AUTOGEN tools to a target: AUTOMOC/UIC/RCC
function(qt_autogen_tools target)
    qt_parse_all_arguments(arg "qt_autogen_tools" "" "" "${__default_private_args}" ${ARGN})

    if (arg_ENABLE_AUTOGEN_TOOLS)
        foreach(tool ${arg_ENABLE_AUTOGEN_TOOLS})
            qt_enable_autogen_tool(${target} ${tool} ON)
        endforeach()
    endif()

  if (arg_DISABLE_AUTOGEN_TOOLS)
      foreach(tool ${arg_DISABLE_AUTOGEN_TOOLS})
          qt_enable_autogen_tool(${target} ${tool} OFF)
      endforeach()
  endif()
endfunction()

# This function stores the list of Qt modules a library depend on,
# along with their version info, for usage in ${target}Depends.cmake file
function(qt_register_target_dependencies target public_libs private_libs)
    get_target_property(target_deps "${target}" _qt_target_deps)
    if(NOT target_deps)
        set(target_deps "")
    endif()

    foreach(lib IN LISTS public_libs private_libs)
        if ("${lib}" MATCHES "^Qt::(.*)")
            set(lib "${CMAKE_MATCH_1}")
            if (lib STREQUAL Platform
                    OR lib STREQUAL GlobalConfig
                    OR lib STREQUAL GlobalConfigPrivate
                    OR lib STREQUAL PlatformModuleInternal
                    OR lib STREQUAL PlatformPluginInternal
                    OR lib STREQUAL PlatformToolInternal)
                list(APPEND target_deps "Qt6\;${PROJECT_VERSION}")
            elseif ("${lib}" MATCHES "(.*)Private")
                list(APPEND target_deps "${INSTALL_CMAKE_NAMESPACE}${CMAKE_MATCH_1}\;${PROJECT_VERSION}")
            else()
                list(APPEND target_deps "${INSTALL_CMAKE_NAMESPACE}${lib}\;${PROJECT_VERSION}")
            endif()
        endif()
    endforeach()

    set_target_properties("${target}" PROPERTIES _qt_target_deps "${target_deps}")
endfunction()

function(qt_update_precompiled_header target precompiled_header)
    if (precompiled_header AND BUILD_WITH_PCH)
        set_property(TARGET "${target}" APPEND PROPERTY "PRECOMPILE_HEADERS" "$<$<COMPILE_LANGUAGE:CXX>:${precompiled_header}>")
    endif()
endfunction()

function(qt_update_precompiled_header_with_library target library)
    if (TARGET "${library}")
        get_target_property(TARGET_TYPE "${library}" TYPE)
        if (NOT TARGET_TYPE STREQUAL "INTERFACE_LIBRARY")
            get_target_property(HEADER "${library}" MODULE_HEADER)
            qt_update_precompiled_header("${target}" "${HEADER}")
        endif()
    endif()
endfunction()

function(qt_update_ignore_pch_source target sources)
    if (sources)
        set_source_files_properties(${sources} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
    endif()
endfunction()

function(qt_ignore_pch_obj_c_sources target sources)
    list(FILTER sources INCLUDE REGEX "\\.mm$")
    qt_update_ignore_pch_source("${target}" "${sources}")
endfunction()

# This function can be used to add sources/libraries/etc. to the specified CMake target
# if the provided CONDITION evaluates to true.
function(extend_target target)
    # Don't try to extend_target when cross compiling an imported host target (like a tool).
    qt_is_imported_target("${target}" is_imported)
    if(is_imported)
        return()
    endif()

    if (NOT TARGET "${target}")
        message(FATAL_ERROR "Trying to extend non-existing target \"${target}\".")
    endif()
    qt_parse_all_arguments(arg "extend_target" "HEADER_MODULE" "PRECOMPILED_HEADER"
        "CONDITION;${__default_public_args};${__default_private_args};COMPILE_FLAGS;NO_PCH_SOURCES" ${ARGN})
    if ("x${arg_CONDITION}" STREQUAL x)
        set(arg_CONDITION ON)
    endif()

    qt_evaluate_config_expression(result ${arg_CONDITION})
    if (${result})
        if(QT_CMAKE_DEBUG_EXTEND_TARGET)
            message("extend_target(${target} CONDITION ${arg_CONDITION} ...): Evaluated")
        endif()
        set(dbus_sources "")
        foreach(adaptor ${arg_DBUS_ADAPTOR_SOURCES})
            qt_create_qdbusxml2cpp_command("${target}" "${adaptor}" ADAPTOR BASENAME "${arg_DBUS_ADAPTOR_BASENAME}" FLAGS "${arg_DBUS_ADAPTOR_FLAGS}")
            list(APPEND dbus_sources "${sources}")
        endforeach()

        foreach(interface ${arg_DBUS_INTERFACE_SOURCES})
            qt_create_qdbusxml2cpp_command("${target}" "${interface}" INTERFACE BASENAME "${arg_DBUS_INTERFACE_BASENAME}" FLAGS "${arg_DBUS_INTERFACE_FLAGS}")
            list(APPEND dbus_sources "${sources}")
        endforeach()

        get_target_property(target_type ${target} TYPE)
        set(is_library FALSE)
        if (${target_type} STREQUAL "STATIC_LIBRARY" OR ${target_type} STREQUAL "SHARED_LIBRARY")
            set(is_library TRUE)
        endif()
        foreach(lib ${arg_PUBLIC_LIBRARIES} ${arg_LIBRARIES})
            # Automatically generate PCH for 'target' using dependencies
            # if 'target' is a library/module!
            if (${is_library})
                qt_update_precompiled_header_with_library("${target}" "${lib}")
            endif()

            string(REGEX REPLACE "_nolink$" "" base_lib "${lib}")
            if(NOT base_lib STREQUAL lib)
                qt_create_nolink_target("${base_lib}" ${target})
            endif()
        endforeach()

        # Find dependencies to internal libraries
        get_target_property(target_deps "${target}" _qt_target_deps)
        if(NOT target_deps)
            set(target_deps "")
        endif()

        foreach(lib ${arg_LIBRARIES})
            if (TARGET "${lib}")
                get_target_property(_is_exported "${lib}" INTERFACE_QT_EXPORTED_LIBRARY)
                if("${_is_exported}")
                    list(APPEND target_deps "${lib}\;${PROJECT_VERSION}")
                endif()
            endif()
        endforeach()
        # Set-up the target
        target_sources("${target}" PRIVATE ${arg_SOURCES} ${dbus_sources})
        if (arg_COMPILE_FLAGS)
            set_source_files_properties(${arg_SOURCES} PROPERTIES COMPILE_FLAGS "${arg_COMPILE_FLAGS}")
        endif()

        set(public_visibility_option "PUBLIC")
        set(private_visibility_option "PRIVATE")
        if(arg_HEADER_MODULE)
            set(public_visibility_option "INTERFACE")
            set(private_visibility_option "INTERFACE")
        endif()
        target_include_directories("${target}"
                                   ${public_visibility_option} ${arg_PUBLIC_INCLUDE_DIRECTORIES}
                                   ${private_visibility_option} ${arg_INCLUDE_DIRECTORIES})
        target_compile_definitions("${target}"
                                    ${public_visibility_option} ${arg_PUBLIC_DEFINES}
                                    ${private_visibility_option} ${arg_DEFINES})
        target_link_libraries("${target}"
                              ${public_visibility_option} ${arg_PUBLIC_LIBRARIES}
                              ${private_visibility_option} ${arg_LIBRARIES})
        target_compile_options("${target}"
                               ${public_visibility_option} ${arg_PUBLIC_COMPILE_OPTIONS}
                               ${private_visibility_option} ${arg_COMPILE_OPTIONS})
        target_link_options("${target}"
                            ${public_visibility_option} ${arg_PUBLIC_LINK_OPTIONS}
                            ${private_visibility_option} ${arg_LINK_OPTIONS})

        if(NOT arg_HEADER_MODULE)
            set_target_properties("${target}" PROPERTIES
                AUTOMOC_MOC_OPTIONS "${arg_MOC_OPTIONS}"
                _qt_target_deps "${target_deps}"
            )
        endif()

        # When computing the private library dependencies, we need to check not only the known
        # modules added by this repo's qt_build_repo(), but also all module dependencies that
        # were found via find_package().
        qt_internal_get_qt_all_known_modules(known_modules)

        # When a public module depends on a private module (Gui on CorePrivate)
        # make its private module depend on the other private module (GuiPrivate will depend on
        # CorePrivate).
        set(qt_libs_private "")
        foreach(it ${known_modules})
            list(FIND arg_LIBRARIES "Qt::${it}Private" pos)
            if(pos GREATER -1)
                list(APPEND qt_libs_private "Qt::${it}Private")
            endif()
        endforeach()

        set(target_private "${target}Private")
        if(TARGET "${target_private}")
          target_link_libraries("${target_private}" INTERFACE "${target}" "${qt_libs_private}")
        endif()
        qt_register_target_dependencies("${target}"
                                        "${arg_PUBLIC_LIBRARIES}"
                                        "${qt_libs_private};${arg_LIBRARIES}")


        qt_autogen_tools(${target}
                         ENABLE_AUTOGEN_TOOLS ${arg_ENABLE_AUTOGEN_TOOLS}
                         DISABLE_AUTOGEN_TOOLS ${arg_DISABLE_AUTOGEN_TOOLS})

        qt_update_precompiled_header("${target}" "${arg_PRECOMPILED_HEADER}")
        qt_update_ignore_pch_source("${target}" "${arg_NO_PCH_SOURCES}")
        ## Ignore objective-c files for PCH (not supported atm)
        qt_ignore_pch_obj_c_sources("${target}" "${arg_SOURCES}")

    else()
        if(QT_CMAKE_DEBUG_EXTEND_TARGET)
            message("extend_target(${target} CONDITION ${arg_CONDITION} ...): Skipped")
        endif()
    endif()
endfunction()


function(qt_internal_library_deprecation_level result)
    if(WIN32)
        # On Windows, due to the way DLLs work, we need to export all functions,
        # including the inlines
        set("${result}" "QT_DISABLE_DEPRECATED_BEFORE=0x040800" PARENT_SCOPE)
    else()
        # On other platforms, Qt's own compilation goes needs to compile the Qt 5.0 API
        set("${result}" "QT_DISABLE_DEPRECATED_BEFORE=0x050000" PARENT_SCOPE)
    endif()
    set("${result}" "QT_DEPRECATED_WARNINGS_SINCE=0x060000" PARENT_SCOPE)
endfunction()


function(qt_install_injections module build_dir install_dir)
    set(injections ${ARGN})
    # examples:
    #  SYNCQT.INJECTIONS = src/corelib/global/qconfig.h:qconfig.h:QtConfig src/corelib/global/qconfig_p.h:5.12.0/QtCore/private/qconfig_p.h
    #  SYNCQT.INJECTIONS = src/gui/vulkan/qvulkanfunctions.h:^qvulkanfunctions.h:QVulkanFunctions:QVulkanDeviceFunctions src/gui/vulkan/qvulkanfunctions_p.h:^5.12.0/QtGui/private/qvulkanfunctions_p.h
    # The are 3 parts to the assignment, divded by colons ':'.
    # The first part contains a path to a generated file in a build folder.
    # The second part contains the file name that the forwarding header should have, which points
    # to the file in the first part.
    # The third part contains multiple UpperCaseFileNames that should be forwarding headers to the
    # header specified in the second part.
    separate_arguments(injections UNIX_COMMAND "${injections}")
    foreach(injection ${injections})
        string(REPLACE ":" ";" injection ${injection})
        # Part 1.
        list(GET injection 0 file)
        # Part 2.
        list(GET injection 1 destination)
        string(REGEX REPLACE "^\\^" "" destination "${destination}")
        list(REMOVE_AT injection 0 1)
        # Part 3.
        set(fwd_hdrs ${injection})
        get_filename_component(destinationdir ${destination} DIRECTORY)
        get_filename_component(destinationname ${destination} NAME)
        get_filename_component(original_file_name ${file} NAME)

        # This describes a concrete example for easier comprehension:
        # A file 'qtqml-config.h' is generated by qt_internal_feature_write_file into
        # ${qtdeclarative_build_dir}/src/{module}/qtqml-config.h (part 1).
        #
        # Generate a lower case forwarding header (part 2) 'qtqml-config.h' at the following
        # location:
        # ${some_prefix}/include/${module}/qtqml-config.h.
        #
        # Inside this file, we #include the originally generated file,
        # ${qtdeclarative_build_dir}/src/{module}/qtqml-config.h.
        #
        # ${some_prefix}'s value depends on the build type.
        # If doing a prefix build, it should point to
        # ${current_repo_build_dir} which is ${qtdeclarative_build_dir}.
        # If doing a non-prefix build, it should point to
        # ${qtbase_build_dir}.
        #
        # In the code below, ${some_prefix} == ${build_dir}.
        set(lower_case_forwarding_header_path "${build_dir}/${INSTALL_INCLUDEDIR}/${module}")
        if(destinationdir)
            string(APPEND lower_case_forwarding_header_path "/${destinationdir}")
        endif()
        set(current_repo_build_dir "${PROJECT_BINARY_DIR}")

        file(RELATIVE_PATH relpath
                           "${lower_case_forwarding_header_path}"
                           "${current_repo_build_dir}/${file}")
        set(main_contents "#include \"${relpath}\"")
        file(GENERATE OUTPUT "${lower_case_forwarding_header_path}/${original_file_name}"
             CONTENT "${main_contents}")

        # Copy the actual injected (generated) header file (not the just created forwarding one)
        # to its install location when doing a prefix build. In an non-prefix build, the qt_install
        # will be a no-op.
        qt_path_join(install_destination
                     ${install_dir} ${INSTALL_INCLUDEDIR} ${module} ${destinationdir})
        qt_install(FILES ${current_repo_build_dir}/${file}
                   DESTINATION ${install_destination}
                   RENAME ${destinationname} OPTIONAL)

        # Generate UpperCaseNamed forwarding headers (part 3).
        foreach(fwd_hdr ${fwd_hdrs})
            set(upper_case_forwarding_header_path "${INSTALL_INCLUDEDIR}/${module}")
            if(destinationdir)
                string(APPEND upper_case_forwarding_header_path "/${destinationdir}")
            endif()

            # Generate upper case forwarding header like QVulkanFunctions or QtConfig.
            file(GENERATE OUTPUT "${build_dir}/${upper_case_forwarding_header_path}/${fwd_hdr}"
                 CONTENT "#include \"${destinationname}\"\n")

            # Install the forwarding header.
            qt_path_join(install_destination
                         ${install_dir} ${upper_case_forwarding_header_path})
            qt_install(FILES "${build_dir}/${upper_case_forwarding_header_path}/${fwd_hdr}"
                       DESTINATION ${install_destination} OPTIONAL)
        endforeach()
    endforeach()
endfunction()


function(qt_read_headers_pri target resultVarPrefix)
    qt_internal_module_info(module "${target}")
    file(STRINGS "${module_include_dir}/headers.pri" headers_pri_contents)
    foreach(line ${headers_pri_contents})
        if("${line}" MATCHES "SYNCQT.HEADER_FILES = (.*)")
            set(public_module_headers "${CMAKE_MATCH_1}")
            separate_arguments(public_module_headers UNIX_COMMAND "${public_module_headers}")
        elseif("${line}" MATCHES "SYNCQT.PRIVATE_HEADER_FILES = (.*)")
            set(private_module_headers "${CMAKE_MATCH_1}")
            separate_arguments(private_module_headers UNIX_COMMAND "${private_module_headers}")
        elseif("${line}" MATCHES "SYNCQT.GENERATED_HEADER_FILES = (.*)")
            set(generated_module_headers "${CMAKE_MATCH_1}")
            separate_arguments(generated_module_headers UNIX_COMMAND "${generated_module_headers}")
            foreach(generated_header ${generated_module_headers})
                list(APPEND public_module_headers "${module_include_dir}/${generated_header}")
            endforeach()
        elseif("${line}" MATCHES "SYNCQT.INJECTIONS = (.*)")
            set(injections "${CMAKE_MATCH_1}")
        elseif("${line}" MATCHES "SYNCQT.([A-Z_]+)_HEADER_FILES = (.+)")
            set(prefix "${CMAKE_MATCH_1}")
            string(TOLOWER "${prefix}" prefix)
            set(entries "${CMAKE_MATCH_2}")
            separate_arguments(entries UNIX_COMMAND "${entries}")
            set("${resultVarPrefix}_${prefix}" "${entries}" PARENT_SCOPE)
        endif()
    endforeach()
    set(${resultVarPrefix}_public "${public_module_headers}" PARENT_SCOPE)
    set(${resultVarPrefix}_private "${private_module_headers}" PARENT_SCOPE)
    set(${resultVarPrefix}_injections "${injections}" PARENT_SCOPE)
endfunction()


# Add Qt::target and Qt6::target as aliases for the target
function(qt_internal_add_target_aliases target)
    get_target_property(type "${target}" TYPE)
    if (type STREQUAL EXECUTABLE)
        add_executable("Qt::${target}" ALIAS "${target}")
        add_executable("Qt${PROJECT_VERSION_MAJOR}::${target}" ALIAS "${target}")
    else()
        add_library("Qt::${target}" ALIAS "${target}")
        add_library("Qt${PROJECT_VERSION_MAJOR}::${target}" ALIAS "${target}")
    endif()
endfunction()

# Sets the exceptions flags for the given target
function(qt_internal_set_no_exceptions_flags target)
    target_compile_definitions("${target}" PRIVATE "QT_NO_EXCEPTIONS")
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        target_compile_options("${target}" PRIVATE "/wd4530" "/wd4577")
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_options("${target}" PRIVATE "-fno-exceptions")
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
        target_compile_options("${target}" PRIVATE "-fno-exceptions")
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        target_compile_options("${target}" PRIVATE "-fno-exceptions")
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
        target_compile_options("${target}" PRIVATE "-fno-exceptions")
    endif()
endfunction()

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
function(add_qt_module target)
    qt_internal_module_info(module "${target}")

    # Process arguments:
    qt_parse_all_arguments(arg "add_qt_module"
        "NO_MODULE_HEADERS;STATIC;DISABLE_TOOLS_EXPORT;EXCEPTIONS;INTERNAL_MODULE;NO_SYNC_QT;NO_PRIVATE_MODULE;HEADER_MODULE"
        "CONFIG_MODULE_NAME;PRECOMPILED_HEADER"
        "${__default_private_args};${__default_public_args};QMAKE_MODULE_CONFIG;EXTRA_CMAKE_FILES;EXTRA_CMAKE_INCLUDES;NO_PCH_SOURCES" ${ARGN})

    if(NOT DEFINED arg_CONFIG_MODULE_NAME)
        set(arg_CONFIG_MODULE_NAME "${module_lower}")
    endif()

    qt_internal_add_qt_repo_known_module("${target}")

    ### Define Targets:
    if(${arg_HEADER_MODULE})
        add_library("${target}" INTERFACE)
    elseif(${arg_STATIC})
        add_library("${target}" STATIC)
    elseif(${QT_BUILD_SHARED_LIBS})
        add_library("${target}" SHARED)
    else()
        add_library("${target}" STATIC)
    endif()

    if (ANDROID)
        qt_android_apply_arch_suffix("${target}")
    endif()
    qt_internal_add_target_aliases("${target}")

    # Add _private target to link against the private headers:
    if(NOT ${arg_NO_PRIVATE_MODULE})
        set(target_private "${target}Private")
        add_library("${target_private}" INTERFACE)
        qt_internal_add_target_aliases("${target_private}")
    endif()

    # Module headers:
    if(${arg_NO_MODULE_HEADERS} OR ${arg_NO_SYNC_QT})
        set_target_properties("${target}" PROPERTIES INTERFACE_MODULE_HAS_HEADERS OFF)
    else()
        # Use QT_BUILD_DIR for the syncqt call.
        # So we either write the generated files into the qtbase non-prefix build root, or the
        # module specific build root.
        qt_ensure_sync_qt()
        set(syncqt_full_command "${HOST_PERL}" -w "${QT_SYNCQT}"
                                 -quiet
                                 -check-includes
                                 -module "${module}"
                                 -version "${PROJECT_VERSION}"
                                 -outdir "${QT_BUILD_DIR}"
                                 -builddir "${PROJECT_BINARY_DIR}"
                                 "${PROJECT_SOURCE_DIR}")
        execute_process(COMMAND ${syncqt_full_command})

        set_target_properties("${target}" PROPERTIES INTERFACE_MODULE_HAS_HEADERS ON)

        ### FIXME: Can we replace headers.pri?
        qt_read_headers_pri("${target}" "module_headers")
        set_property(TARGET "${target}" APPEND PROPERTY PUBLIC_HEADER "${module_headers_public}")
        set_property(TARGET "${target}" APPEND PROPERTY PUBLIC_HEADER "${module_include_dir}/${module}Depends")
        set_property(TARGET "${target}" APPEND PROPERTY PRIVATE_HEADER "${module_headers_private}")
        if (NOT ${arg_HEADER_MODULE})
            set_property(TARGET "${target}" PROPERTY MODULE_HEADER "${module_include_dir}/${module}")
        endif()

        if(module_headers_qpa)
            qt_install(FILES ${module_headers_qpa} DESTINATION ${INSTALL_INCLUDEDIR}/${module}/${PROJECT_VERSION}/${module}/qpa)
        endif()
    endif()

    if(NOT arg_HEADER_MODULE)
        # Plugin types associated to a module
        if(NOT "x${arg_PLUGIN_TYPES}" STREQUAL "x")
            # Reset the variable containing the list of plugins for the given plugin type
            foreach(plugin_type ${arg_PLUGIN_TYPES})
                # Used to handle some edge cases such as platforms/darwin
                string(REGEX REPLACE "[-/]" "_" plugin_type "${plugin_type}")

                set_property(TARGET "${target}" APPEND PROPERTY MODULE_PLUGIN_TYPES "${plugin_type}")
                qt_internal_add_qt_repo_known_plugin_types("${plugin_type}")
            endforeach()
        endif()

        set_target_properties("${target}" PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY "${QT_BUILD_DIR}/${INSTALL_LIBDIR}"
            RUNTIME_OUTPUT_DIRECTORY "${QT_BUILD_DIR}/${INSTALL_BINDIR}"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${QT_BUILD_DIR}/${INSTALL_BINDIR}"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${QT_BUILD_DIR}/${INSTALL_BINDIR}"
            ARCHIVE_OUTPUT_DIRECTORY "${QT_BUILD_DIR}/${INSTALL_LIBDIR}"
            VERSION ${PROJECT_VERSION}
            SOVERSION ${PROJECT_VERSION_MAJOR}
            OUTPUT_NAME "${INSTALL_CMAKE_NAMESPACE}${target}"
        )
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

    # Handle cases like QmlDevTools which do not have their own headers, but rather borrow them
    # from another module.
    if(NOT arg_NO_SYNC_QT AND NOT arg_NO_MODULE_HEADERS)
        list(APPEND private_includes
                    "$<BUILD_INTERFACE:${module_include_dir}/${PROJECT_VERSION}>"
                    "$<BUILD_INTERFACE:${module_include_dir}/${PROJECT_VERSION}/${module}>")

        list(APPEND public_includes
                    # For the syncqt headers
                    "$<BUILD_INTERFACE:${module_repo_include_dir}>"
                    "$<BUILD_INTERFACE:${module_include_dir}>")
    endif()

    if(NOT arg_NO_MODULE_HEADERS AND NOT arg_NO_SYNC_QT)
        # For the syncqt headers
        list(APPEND public_includes "$<INSTALL_INTERFACE:include/${module}>")
    endif()
    list(APPEND public_includes ${arg_PUBLIC_INCLUDE_DIRECTORIES})

    set(header_module)
    if(arg_HEADER_MODULE)
        set(header_module "HEADER_MODULE")
    endif()

    extend_target("${target}"
        ${header_module}
        SOURCES ${arg_SOURCES}
        INCLUDE_DIRECTORIES
            ${private_includes}
        PUBLIC_INCLUDE_DIRECTORIES
            ${public_includes}
        PUBLIC_DEFINES
            ${arg_PUBLIC_DEFINES}
            QT_${module_define}_LIB
        DEFINES
            ${arg_DEFINES}
            QT_NO_CAST_TO_ASCII QT_ASCII_CAST_WARNINGS
            QT_MOC_COMPAT #we don't need warnings from calling moc code in our generated code
            QT_USE_QSTRINGBUILDER
            QT_DEPRECATED_WARNINGS
            QT_BUILDING_QT
            QT_BUILD_${module_define}_LIB ### FIXME: use QT_BUILD_ADDON for Add-ons or remove if we don't have add-ons anymore
            "${deprecation_define}"
        PUBLIC_LIBRARIES ${arg_PUBLIC_LIBRARIES}
        LIBRARIES ${arg_LIBRARIES} Qt::PlatformModuleInternal
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

    if(NOT ${arg_EXCEPTIONS} AND NOT ${arg_HEADER_MODULE})
        qt_internal_set_no_exceptions_flags("${target}")
    endif()

    set(configureFile "${CMAKE_CURRENT_SOURCE_DIR}/configure.cmake")
    if(EXISTS "${configureFile}")
        qt_feature_module_begin(
            LIBRARY "${target}"
            PUBLIC_FILE "qt${arg_CONFIG_MODULE_NAME}-config.h"
            PRIVATE_FILE "qt${arg_CONFIG_MODULE_NAME}-config_p.h"
            PUBLIC_DEPENDENCIES ${arg_FEATURE_DEPENDENCIES}
            PRIVATE_DEPENDENCIES ${arg_FEATURE_DEPENDENCIES}
        )
        include(${configureFile})
        qt_feature_module_end("${target}")

        set_property(TARGET "${target}" APPEND PROPERTY PUBLIC_HEADER "${CMAKE_CURRENT_BINARY_DIR}/qt${arg_CONFIG_MODULE_NAME}-config.h")
        set_property(TARGET "${target}" APPEND PROPERTY PRIVATE_HEADER "${CMAKE_CURRENT_BINARY_DIR}/qt${arg_CONFIG_MODULE_NAME}-config_p.h")
    endif()

    if(DEFINED module_headers_private)
        qt_internal_add_linker_version_script("${target}" PRIVATE_HEADERS ${module_headers_private} ${module_headers_qpa})
    else()
        qt_internal_add_linker_version_script("${target}")
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
        qt_install_injections("${module}" "${QT_BUILD_DIR}" "${QT_INSTALL_DIR}" ${final_injections})
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

    if(target STREQUAL Core)
        # Propagate non-build related variables that are needed for consuming Qt packages.
        # Do this in CoreConfig instead of Qt6Config, so that consumers can also use
        # find_package(Qt6Core) instead of find_package(Qt6 COMPONENTS Core)
        string(APPEND extra_cmake_code "
set(QT_CMAKE_EXPORT_NAMESPACE ${QT_CMAKE_EXPORT_NAMESPACE})")
    endif()

    configure_package_config_file(
        "${QT_CMAKE_DIR}/QtModuleConfig.cmake.in"
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
    qt_install(TARGETS ${exported_targets}
        EXPORT ${export_name}
        RUNTIME DESTINATION ${INSTALL_BINDIR}
        LIBRARY DESTINATION ${INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${INSTALL_INCLUDEDIR}/${module}
        PRIVATE_HEADER DESTINATION ${INSTALL_INCLUDEDIR}/${module}/${PROJECT_VERSION}/${module}/private
        )

    if (ANDROID)
        # Record install library location so it can be accessed by
        # qt_android_dependencies without having to specify it again.
        set_target_properties(${target} PROPERTIES
            QT_ANDROID_MODULE_INSTALL_DIR ${INSTALL_LIBDIR})
    endif()

    qt_install(EXPORT ${export_name}
               NAMESPACE ${QT_CMAKE_EXPORT_NAMESPACE}::
               DESTINATION ${config_install_dir})

    qt_internal_export_modern_cmake_config_targets_file(
        TARGETS ${exported_targets}
        EXPORT_NAME_PREFIX ${INSTALL_CMAKE_NAMESPACE}${target}
        CONFIG_INSTALL_DIR "${config_install_dir}")

    if (${arg_INTERNAL_MODULE})
        set(arg_INTERNAL_MODULE "INTERNAL_MODULE")
    else()
        unset(arg_INTERNAL_MODULE)
    endif()

    qt_path_join(pri_target_path ${PROJECT_BINARY_DIR} mkspecs/modules)
    qt_generate_module_pri_file("${target}" "${pri_target_path}" module_pri_files
        ${arg_INTERNAL_MODULE}
        ${header_module}
        QMAKE_MODULE_CONFIG
            ${arg_QMAKE_MODULE_CONFIG}
        )
    qt_install(FILES "${module_pri_files}" DESTINATION mkspecs/modules)

    ### fixme: cmake is missing a built-in variable for this. We want to apply it only to modules and plugins
    # that belong to Qt.
    if(NOT arg_HEADER_MODULE)
        qt_internal_add_link_flags_no_undefined("${target}")
    endif()

    set(interface_includes "")

    # Handle cases like QmlDevTools which do not have their own headers, but rather borrow them
    # from another module.
    if(NOT arg_NO_SYNC_QT)
        list(APPEND interface_includes
                    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
                    "$<BUILD_INTERFACE:${module_include_dir}/${PROJECT_VERSION}>"
                    "$<BUILD_INTERFACE:${module_include_dir}/${PROJECT_VERSION}/${module}>")

        if(NOT arg_NO_MODULE_HEADERS)
            list(APPEND interface_includes
                        "$<INSTALL_INTERFACE:include/${module}/${PROJECT_VERSION}>"
                        "$<INSTALL_INTERFACE:include/${module}/${PROJECT_VERSION}/${module}>")
        endif()
    endif()

    if(NOT ${arg_NO_PRIVATE_MODULE})
        target_include_directories("${target_private}" INTERFACE ${interface_includes})
    endif()

    qt_describe_module(${target})
endfunction()

function(qt_export_tools module_name)
    # If no tools were defined belonging to this module, don't create a config and targets file.
    # Guards against the case when doing a cross-build.

    if(NOT "${module_name}" IN_LIST QT_KNOWN_MODULES_WITH_TOOLS OR CMAKE_CROSSCOMPILING)
        return()
    endif()

    # The tools target name. For example: CoreTools
    set(target "${module_name}Tools")

    set(path_suffix "${INSTALL_CMAKE_NAMESPACE}${target}")
    qt_path_join(config_build_dir ${QT_CONFIG_BUILD_DIR} ${path_suffix})
    qt_path_join(config_install_dir ${QT_CONFIG_INSTALL_DIR} ${path_suffix})

    # Add the extra cmake statements to make the tool targets global, so it doesn't matter where
    # find_package is called.
    # Also assemble a list of tool targets to expose in the config file for informational purposes.
    set(extra_cmake_statements "")
    set(tool_targets "")
    foreach(tool_name ${QT_KNOWN_MODULE_${module_name}_TOOLS})
        set(extra_cmake_statements "${extra_cmake_statements}
if (NOT QT_NO_CREATE_TARGETS)
    get_property(is_global TARGET ${INSTALL_CMAKE_NAMESPACE}::${tool_name} PROPERTY IMPORTED_GLOBAL)
    if(NOT is_global)
        set_property(TARGET ${INSTALL_CMAKE_NAMESPACE}::${tool_name} PROPERTY IMPORTED_GLOBAL TRUE)
    endif()
endif()
")
        list(APPEND tool_targets "${QT_CMAKE_EXPORT_NAMESPACE}::${tool_name}")
    endforeach()

    string(APPEND extra_cmake_statements
"set(${QT_CMAKE_EXPORT_NAMESPACE}${module_name}Tools_TARGETS \"${tool_targets}\")")

    configure_package_config_file(
        "${QT_CMAKE_DIR}/QtModuleToolsConfig.cmake.in"
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

    set(export_name "${INSTALL_CMAKE_NAMESPACE}${target}Targets")
    qt_install(EXPORT "${export_name}"
               NAMESPACE "${QT_CMAKE_EXPORT_NAMESPACE}::"
               DESTINATION "${config_install_dir}")

    # Temporarily disable creation of versionless targets for tools,
    # because it breaks qtdeclarative build.
    #qt_internal_export_modern_cmake_config_targets_file(TARGETS ${QT_KNOWN_MODULE_${module_name}_TOOLS}
    #                                                    EXPORT_NAME_PREFIX ${INSTALL_CMAKE_NAMESPACE}${target}
    #                                                    CONFIG_INSTALL_DIR ${config_install_dir})
endfunction()

function(qt_internal_check_directory_or_type name dir type default result_var)
    if ("x${dir}" STREQUAL x)
        if("x${type}" STREQUAL x)
            message(FATAL_ERROR "add_qt_plugin called without setting either TYPE or ${name}.")
        endif()
        set(${result_var} "${default}" PARENT_SCOPE)
    else()
        set(${result_var} "${dir}" PARENT_SCOPE)
    endif()
endfunction()

# Utility function to find the module to which a plug-in belongs.
# This will set the QT_MODULE target property on the plug-in - e.g. "Gui", "Sql"...
function(qt_get_module_for_plugin target target_type)
    qt_internal_get_qt_all_known_modules(known_modules)
    foreach(qt_module ${known_modules})
        get_target_property(module_type "${QT_CMAKE_EXPORT_NAMESPACE}::${qt_module}" TYPE)
        # Assuming interface libraries can't have plugins. Otherwise we'll need to fix the property
        # name, because the current one would be invalid for interface libraries.
        if(module_type STREQUAL "INTERFACE_LIBRARY")
            continue()
        endif()

        get_target_property(plugin_types
                           "${QT_CMAKE_EXPORT_NAMESPACE}::${qt_module}"
                            MODULE_PLUGIN_TYPES)
        if(plugin_types)
            foreach(plugin_type ${plugin_types})
                if("${target_type}" STREQUAL "${plugin_type}")
                    set_target_properties("${target}" PROPERTIES QT_MODULE "${qt_module}")
                    return()
                endif()
            endforeach()
        endif()
    endforeach()
    message(AUTHOR_WARNING "The plug-in '${target}' does not belong to any Qt module.")
endfunction()


# Collection of add_qt_plugin arguments so they can be shared across different
# plugin type wrappers
set(__add_qt_plugin_optional_args
    "STATIC;EXCEPTIONS;ALLOW_UNDEFINED_SYMBOLS"
)
set(__add_qt_plugin_single_args
    "TYPE;CLASS_NAME;OUTPUT_DIRECTORY;INSTALL_DIRECTORY;ARCHIVE_INSTALL_DIRECTORY;QML_TARGET_PATH"
)
set(__add_qt_plugin_multi_args
    "${__default_private_args};${__default_public_args};DEFAULT_IF"
)
# This is the main entry point for defining Qt plugins.
# A CMake target is created with the given target. The TYPE parameter is needed to place the
# plugin into the correct plugins/ sub-directory.
function(add_qt_plugin target)
    qt_internal_module_info(module "${target}")

    qt_internal_set_qt_known_plugins("${QT_KNOWN_PLUGINS}" "${target}")

    qt_parse_all_arguments(arg "add_qt_plugin"
        "${__add_qt_plugin_optional_args};SKIP_INSTALL"
        "${__add_qt_plugin_single_args}"
        "${__add_qt_plugin_multi_args}"
        "${ARGN}"
    )

    set(output_directory_default "${QT_BUILD_DIR}/${INSTALL_PLUGINSDIR}/${arg_TYPE}")
    set(install_directory_default "${INSTALL_PLUGINSDIR}/${arg_TYPE}")
    set(archive_install_directory_default "${INSTALL_LIBDIR}/${arg_TYPE}")

    if (arg_QML_TARGET_PATH)
        set(target_path "${arg_QML_TARGET_PATH}")
        set(output_directory_default "${QT_BUILD_DIR}/${INSTALL_QMLDIR}/${target_path}")
        set(install_directory_default "${INSTALL_QMLDIR}/${target_path}")
        set(archive_install_directory_default "${INSTALL_QMLDIR}/${target_path}")
    endif()

    if ("x${arg_CLASS_NAME}" STREQUAL "x" AND NOT "${arg_TYPE}" STREQUAL "qml_plugin")
        message(AUTHOR_WARNING "add_qt_plugin called without setting CLASS_NAME.")
    endif()

    qt_internal_check_directory_or_type(OUTPUT_DIRECTORY "${arg_OUTPUT_DIRECTORY}" "${arg_TYPE}"
        "${output_directory_default}" output_directory)
    if (NOT arg_SKIP_INSTALL)
        qt_internal_check_directory_or_type(INSTALL_DIRECTORY "${arg_INSTALL_DIRECTORY}" "${arg_TYPE}"
            "${install_directory_default}" install_directory)
        if (NOT arg_ARCHIVE_INSTALL_DIRECTORY AND arg_INSTALL_DIRECTORY)
            set(arg_ARCHIVE_INSTALL_DIRECTORY "${arg_INSTALL_DIRECTORY}")
        endif()
    endif()

    if(arg_STATIC OR NOT BUILD_SHARED_LIBS)
        add_library("${target}" STATIC)
    else()
        add_library("${target}" MODULE)
        if(APPLE)
            # CMake defaults to using .so extensions for loadable modules, aka plugins,
            # but Qt plugins are actually suffixed with .dylib.
            set_property(TARGET "${target}" PROPERTY SUFFIX ".dylib")
        endif()
    endif()

    if (ANDROID)
        qt_android_apply_arch_suffix("${target}")
    endif()
    qt_internal_add_target_aliases("${target}")

    set_target_properties("${target}" PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${output_directory}"
        RUNTIME_OUTPUT_DIRECTORY "${output_directory}"
        ARCHIVE_OUTPUT_DIRECTORY "${output_directory}"
        QT_PLUGIN_CLASS_NAME "${arg_CLASS_NAME}")

    qt_internal_library_deprecation_level(deprecation_define)

    qt_autogen_tools_initial_setup(${target})

    set(static_plugin_define "")
    if (arg_STATIC OR NOT QT_BUILD_SHARED_LIBS)
        set(static_plugin_define "QT_STATICPLUGIN")
    endif()

    # Save the Qt module in the plug-in's properties
    if(NOT arg_TYPE STREQUAL "qml_plugin")
        qt_get_module_for_plugin("${target}" "${arg_TYPE}")
        get_target_property(qt_module "${target}" QT_MODULE)
    endif()

    # Add the plug-in to the list of plug-ins of this module
    if(TARGET "${qt_module}")
        set_property(TARGET "${qt_module}" APPEND PROPERTY QT_PLUGINS "${target}")
    endif()

    set(_default_plugin 1)
    if (DEFINED arg_DEFAULT_IF)
      if (NOT ${arg_DEFAULT_IF})
          set(_default_plugin 0)
      endif()
    endif()

    set_property(TARGET "${target}" PROPERTY QT_DEFAULT_PLUGIN "${_default_plugin}")
    set_property(TARGET "${target}" APPEND PROPERTY EXPORT_PROPERTIES "QT_PLUGIN_CLASS_NAME;QT_PLUGIN_TYPE;QT_MODULE;QT_DEFAULT_PLUGIN")

    set(private_includes
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}"
         # For the syncqt headers
        "$<BUILD_INTERFACE:${module_repo_include_dir}>"
         ${arg_INCLUDE_DIRECTORIES}
    )

    set(public_includes
        ${arg_PUBLIC_INCLUDE_DIRECTORIES}
    )

    extend_target("${target}"
        SOURCES ${arg_SOURCES}
        INCLUDE_DIRECTORIES
            ${private_includes}
        PUBLIC_INCLUDE_DIRECTORIES
            ${public_includes}
        LIBRARIES ${arg_LIBRARIES} Qt::PlatformPluginInternal
        PUBLIC_LIBRARIES ${arg_PUBLIC_LIBRARIES}
        DEFINES
            ${arg_DEFINES}
            QT_DEPRECATED_WARNINGS
            "${deprecation_define}"
            "${static_plugin_define}"
            QT_PLUGIN
        PUBLIC_DEFINES
            QT_${module_define}_LIB
            ${arg_PUBLIC_DEFINES}
        FEATURE_DEPENDENCIES ${arg_FEATURE_DEPENDENCIES}
        DBUS_ADAPTOR_SOURCES "${arg_DBUS_ADAPTOR_SOURCES}"
        DBUS_ADAPTOR_FLAGS "${arg_DBUS_ADAPTOR_FLAGS}"
        DBUS_INTERFACE_SOURCES "${arg_DBUS_INTERFACE_SOURCES}"
        DBUS_INTERFACE_FLAGS "${arg_DBUS_INTERFACE_FLAGS}"
        COMPILE_OPTIONS ${arg_COMPILE_OPTIONS}
        PUBLIC_COMPILE_OPTIONS ${arg_PUBLIC_COMPILE_OPTIONS}
        LINK_OPTIONS ${arg_LINK_OPTIONS}
        PUBLIC_LINK_OPTIONS ${arg_PUBLIC_LINK_OPTIONS}
        MOC_OPTIONS ${arg_MOC_OPTIONS}
        ENABLE_AUTOGEN_TOOLS ${arg_ENABLE_AUTOGEN_TOOLS}
        DISABLE_AUTOGEN_TOOLS ${arg_DISABLE_AUTOGEN_TOOLS}
    )
    if(NOT ${arg_EXCEPTIONS})
        qt_internal_set_no_exceptions_flags("${target}")
    endif()


    set(qt_libs_private "")
    qt_internal_get_qt_all_known_modules(known_modules)
    foreach(it ${known_modules})
        list(FIND arg_LIBRARIES "Qt::${it}Private" pos)
        if(pos GREATER -1)
            list(APPEND qt_libs_private "Qt::${it}Private")
        endif()
    endforeach()

    qt_register_target_dependencies("${target}" "${arg_PUBLIC_LIBRARIES}" "${qt_libs_private}")

    if (NOT arg_SKIP_INSTALL)
        # Handle creation of cmake files for consumers of find_package().
        # If we are part of a Qt module, the plugin cmake files are installed as part of that module.
        if(qt_module)
            set(path_suffix "${INSTALL_CMAKE_NAMESPACE}${qt_module}")
        else()
            set(path_suffix "${INSTALL_CMAKE_NAMESPACE}${target}")
        endif()

        qt_path_join(config_build_dir ${QT_CONFIG_BUILD_DIR} ${path_suffix})
        qt_path_join(config_install_dir ${QT_CONFIG_INSTALL_DIR} ${path_suffix})

        configure_package_config_file(
            "${QT_CMAKE_DIR}/QtPluginConfig.cmake.in"
            "${config_build_dir}/${target}Config.cmake"
            INSTALL_DESTINATION "${config_install_dir}"
        )
        write_basic_package_version_file(
            "${config_build_dir}/${target}ConfigVersion.cmake"
            VERSION ${PROJECT_VERSION}
            COMPATIBILITY AnyNewerVersion
        )

        qt_install(FILES
            "${config_build_dir}/${target}Config.cmake"
            "${config_build_dir}/${target}ConfigVersion.cmake"
            DESTINATION "${config_install_dir}"
            COMPONENT Devel
        )

        # Make the export name of plugins be consistent with modules, so that
        # add_qt_resource adds its additional targets to the same export set in a static Qt build.
        set(export_name "${INSTALL_CMAKE_NAMESPACE}${target}Targets")
        qt_install(TARGETS "${target}"
                   EXPORT ${export_name}
                   RUNTIME DESTINATION "${install_directory}"
                   LIBRARY DESTINATION "${install_directory}"
                   ARCHIVE DESTINATION "${archive_install_directory}"
        )
        qt_install(EXPORT ${export_name}
                   NAMESPACE ${QT_CMAKE_EXPORT_NAMESPACE}::
                   DESTINATION "${config_install_dir}"
        )
    endif()

    # Store the plug-in type in the target property
    set_property(TARGET "${target}" PROPERTY QT_PLUGIN_TYPE "${arg_TYPE}")

    if (NOT arg_ALLOW_UNDEFINED_SYMBOLS)
        ### fixme: cmake is missing a built-in variable for this. We want to apply it only to
        # modules and plugins that belong to Qt.
        qt_internal_add_link_flags_no_undefined("${target}")
    endif()

    qt_internal_add_linker_version_script(${target})
endfunction()

# Generate custom ${target}_qmltypes target for Qml Plugins
function(qt_add_qmltypes_target target)

    # Do nothing when cross compiling
    if (CMAKE_CROSSCOMPILING)
        return()
    endif()

    # Do nothing during a Qt static build (because the tool isn't built in that case).
    if (NOT QT_BUILD_SHARED_LIBS)
        return()
    endif()

    qt_parse_all_arguments(arg "qt_generate_qmltypes"
        ""
        "TARGET_PATH;IMPORT_VERSION;IMPORT_NAME;QML_PLUGINDUMP_DEPENDENCIES"
        ""
        ${ARGN})

    # scan repos for qml repositories
    foreach(repo IN LISTS QT_REPOS)
        if (IS_DIRECTORY "${repo}/qml")
            list(APPEND import_paths "${repo}/qml")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES import_paths)
    if (UNIX)
        list(JOIN import_paths ":" import_paths_env)
    else()
        list(JOIN import_paths "\;" import_paths_env)
    endif()

    if(NOT arg_IMPORT_NAME)
        string(REGEX REPLACE "\\.\\d+$" "" import_name ${arg_TARGET_PATH})
    else()
        set(import_name ${arg_IMPORT_NAME})
    endif()

    if(NOT arg_IMPORT_VERSION)
        message(FATAL_ERROR "Import version parameter was not specified. Specify the import version using the IMPORT_VERSION.")
    endif()

    get_target_property(source_dir ${target} SOURCE_DIR)

    # qml1_target check is no longer required
    set(qmltypes_command_args "-nonrelocatable")
    if (NOT arg_QML_PLUGINDUMP_DEPENDENCIES AND EXISTS "${source_dir}/dependencies.json")
        list(APPEND qmltypes_command_args "-dependencies" "${source_dir}/dependencies.json")
    elseif(arg_QML_PLUGINDUMP_DEPENDENCIES)
        list(APPEND qmltypes_command_args "-dependencies" "${arg_QML_PLUGINDUMP_DEPENDENCIES}")
    endif()

    string(REPLACE "/" "." import_name_arg ${import_name})

    list(APPEND qmltypes_command_args "${import_name_arg}" "${arg_IMPORT_VERSION}")

    set(qml_plugindump_target ${QT_CMAKE_EXPORT_NAMESPACE}::qmlplugindump)

    # Manually set dependency on plugindump target since CMake will not add
    # this rule because it's not the main executable argument to the COMMAND
    # parameter.
    add_custom_target(
        "${target}_qmltypes"
        DEPENDS ${qml_plugindump_target}
        COMMAND ${CMAKE_COMMAND} -E env "QML2_IMPORTPATH=${import_paths_env}"
        $<TARGET_FILE:${qml_plugindump_target}> ${qmltypes_command_args} > "${source_dir}/plugins.qmltypes"
        )
endfunction()

function(qt_install_qml_files target)

    qt_parse_all_arguments(arg "qt_install_qml_files"
        "" "" "FILES" ${ARGN}
    )

    if (NOT arg_FILES)
        message(FATAL_ERROR "No files specified for qt_install_qml_files. Please specify them using the FILES parameter.")
    endif()

    get_target_property(target_path ${target} QT_QML_MODULE_TARGET_PATH)
    if (NOT target_path)
        message(FATAL_ERROR "Target ${target} is not a qml module.")
    endif()

    qt_path_join(qml_module_install_dir ${QT_INSTALL_DIR} "${INSTALL_QMLDIR}/${target_path}")
    qt_copy_or_install(FILES ${arg_FILES}
        DESTINATION ${qml_module_install_dir}
    )

endfunction()


function(add_qt_resource target resourceName)
    # Don't try to add resources when cross compiling, and the target is actually a host target
    # (like a tool).
    qt_is_imported_target("${target}" is_imported)
    if(is_imported)
        return()
    endif()

    qt_parse_all_arguments(arg "add_qt_resource" "" "PREFIX;LANG;BASE" "FILES" ${ARGN})

    QT6_PROCESS_RESOURCE(${target} ${resourceName}
        PREFIX "${arg_PREFIX}"
        LANG "${arg_LANG}"
        BASE "${arg_BASE}"
        FILES ${arg_FILES}
        OUTPUT_TARGETS out_targets
   )

   if (out_targets)
        qt_install(TARGETS ${out_targets}
            EXPORT "${INSTALL_CMAKE_NAMESPACE}${target}Targets"
            DESTINATION ${INSTALL_LIBDIR}
        )
   endif()

endfunction()


# This function creates a CMake target for qml modules. It will also make
# sure that if no C++ source are present, that qml files show up in the project
# in an IDE. Finally, it will also create a custom ${target}_qmltypes which
# can be used to generate the respective plugin.qmltypes file.
#
#  CPP_PLUGIN: Whether this qml module has any c++ source files.
#  URI: Module's uri.
#  TARGET_PATH: Expected installation path for the Qml Module. Equivalent
#  to the module's URI where '.' is replaced with '/'. Use this to override the
#  default substitution pattern.
#  VERSION: Version of the qml module
#  QML_PLUGINDUMP_DEPENDENCIES: Path to a dependencies.json file to be consumed
#  with the ${target}_qmltypes target (optional)
#  SKIP_TYPE_REGISTRATION: All qml files are expected to be registered by the
#  c++ plugin code.
#
function(add_qml_module target)

    set(qml_module_optional_args
        DESIGNER_SUPPORTED
        DO_NOT_INSTALL
        SKIP_TYPE_REGISTRATION
    )

    set(qml_module_single_args
        URI
        TARGET_PATH
        VERSION
        QML_PLUGINDUMP_DEPENDENCIES
        CLASSNAME
    )

    set(qml_module_multi_args
        IMPORTS
        TYPEINFO
        DEPENDENCIES
    )

    qt_parse_all_arguments(arg "add_qml_module"
        "${__add_qt_plugin_optional_args};${qml_module_optional_args}"
        "${__add_qt_plugin_single_args};${qml_module_single_args}"
        "${__add_qt_plugin_multi_args};${qml_module_multi_args}" ${ARGN})

    if (NOT arg_URI)
        message(FATAL_ERROR "add_qml_module called without specifying the module's uri. Please specify one using the URI parameter.")
    endif()

    set(target_path ${arg_TARGET_PATH})

    if (NOT arg_VERSION)
        message(FATAL_ERROR "add_qml_module called without specifying the module's import version. Please specify one using the VERSION parameter.")
    endif()

    if (NOT arg_TARGET_PATH)
        string(REPLACE "." "/" arg_TARGET_PATH ${arg_URI})
    endif()

    qt_remove_args(plugin_args
        ARGS_TO_REMOVE
            ${target}
            ${qml_module_multi_args}
            ${qml_module_single_args}
        ALL_ARGS
            ${__add_qt_plugin_optional_args}
            ${__add_qt_plugin_single_args}
            ${qml_module_single_args}
            ${__add_qt_plugin_multi_args}
            ${qml_module_multi_args}
        ARGS
            ${ARGV}
    )

    # If we have no sources, but qml files, create a custom target so the
    # qml file will be visibile in an IDE.
    if (arg_SOURCES)
        add_qt_plugin(${target}
            TYPE
                qml_plugin
            QML_TARGET_PATH
                "${arg_TARGET_PATH}"
            ${plugin_args}
        )
    endif()


    if (arg_CPP_PLUGIN)
        set(no_create_option DO_NOT_CREATE_TARGET)
    endif()

    if (arg_CLASSNAME)
        set(classname_arg CLASSNAME ${arg_CLASSNAME})
    endif()

    if (arg_DESIGNER_SUPPORTED)
        set(designer_supported_arg DESIGNER_SUPPORTED)
    endif()

    if (arg_SKIP_TYPE_REGISTRATION)
        set(skip_registration_arg SKIP_TYPE_REGISTRATION)
    endif()

    qt6_add_qml_module(${target}
        ${designer_supported_arg}
        ${no_create_option}
        ${skip_registration_arg}
        ${classname_arg}
        RESOURCE_PREFIX "/qt-project.org/imports"
        TARGET_PATH ${arg_TARGET_PATH}
        URI ${arg_URI}
        VERSION ${arg_VERSION}
        QML_FILES ${arg_QML_FILES}
        IMPORTS "${arg_IMPORTS}"
        TYPEINFO "${arg_TYPEINFO}"
        DO_NOT_INSTALL_METADATA
        DO_NOT_CREATE_TARGET
        INSTALL_QML_FILES
        DEPENDENCIES ${arg_DEPENDENCIES}
        RESOURCE_EXPORT "${INSTALL_CMAKE_NAMESPACE}${target}Targets"
    )

    get_target_property(qmldir_file ${target} QT_QML_MODULE_QMLDIR_FILE)
    qt_path_join(qml_module_install_dir ${QT_INSTALL_DIR} "${INSTALL_QMLDIR}/${arg_TARGET_PATH}")
    set(plugin_types "${CMAKE_CURRENT_SOURCE_DIR}/plugins.qmltypes")
    if (EXISTS ${plugin_types})
        qt_copy_or_install(FILES ${plugin_types}
            DESTINATION "${qml_module_install_dir}"
        )

        if(QT_WILL_INSTALL)
            # plugin.qmltypes when present should also be copied to the
            # cmake binary dir when doing prefix builds
            file(COPY ${plugin_types}
                DESTINATION "${QT_BUILD_DIR}/${INSTALL_QMLDIR}/${arg_TARGET_PATH}"
            )
        endif()
    endif()


    qt_copy_or_install(
        FILES
            "${qmldir_file}"
        DESTINATION
            "${qml_module_install_dir}"
    )

    if(QT_WILL_INSTALL)
        # qmldir should also be copied to the cmake binary dir when doing
        # prefix builds
        file(COPY "${qmldir_file}"
            DESTINATION "${QT_BUILD_DIR}/${INSTALL_QMLDIR}/${arg_TARGET_PATH}"
        )
    endif()

endfunction()

# Collection of add_qt_executable arguments so they can be shared across add_qt_executable
# and add_qt_test_helper.
set(__add_qt_executable_optional_args
    "GUI;BOOTSTRAP;NO_QT;NO_INSTALL;EXCEPTIONS"
)
set(__add_qt_executable_single_args
    "OUTPUT_DIRECTORY;INSTALL_DIRECTORY"
)
set(__add_qt_executable_multi_args
    "EXE_FLAGS;${__default_private_args};${__default_public_args}"
)

# This function creates a CMake target for a generic console or GUI binary.
# Please consider to use a more specific version target like the one created
# by add_qt_test or add_qt_tool below.
function(add_qt_executable name)
    qt_parse_all_arguments(arg "add_qt_executable"
        "${__add_qt_executable_optional_args}"
        "${__add_qt_executable_single_args}"
        "${__add_qt_executable_multi_args}"
        ${ARGN})

    if ("x${arg_OUTPUT_DIRECTORY}" STREQUAL "x")
        set(arg_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${INSTALL_BINDIR}")
    endif()

    get_filename_component(arg_OUTPUT_DIRECTORY "${arg_OUTPUT_DIRECTORY}"
        ABSOLUTE BASE_DIR "${CMAKE_BINARY_DIR}")

    if ("x${arg_INSTALL_DIRECTORY}" STREQUAL "x")
        set(arg_INSTALL_DIRECTORY "${INSTALL_BINDIR}")
    endif()

    add_executable("${name}" ${arg_EXE_FLAGS})

    qt_autogen_tools_initial_setup(${name})

    set(extra_libraries "")
    if(NOT arg_BOOTSTRAP AND NOT arg_NO_QT)
        set(extra_libraries "Qt::Core")
    endif()

    set(private_includes
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}"
         ${arg_INCLUDE_DIRECTORIES}
    )

    extend_target("${name}"
        SOURCES ${arg_SOURCES}
        INCLUDE_DIRECTORIES ${private_includes}
        DEFINES ${arg_DEFINES}
        LIBRARIES ${arg_LIBRARIES}
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
        WIN32_EXECUTABLE "${arg_GUI}"
        MACOSX_BUNDLE "${arg_GUI}"
    )
    if(NOT ${arg_EXCEPTIONS})
        qt_internal_set_no_exceptions_flags("${name}")
    endif()


    if(NOT arg_NO_INSTALL)
        qt_install(TARGETS "${name}"
            RUNTIME DESTINATION "${arg_INSTALL_DIRECTORY}"
            BUNDLE DESTINATION "${arg_INSTALL_DIRECTORY}")
    endif()
endfunction()


# This function creates a CMake test target with the specified name for use with CTest.
function(add_qt_test name)
    qt_parse_all_arguments(arg "add_qt_test"
        "RUN_SERIAL;EXCEPTIONS;GUI;QMLTEST"
        "OUTPUT_DIRECTORY" "QML_IMPORTPATH;TESTDATA;${__default_private_args};${__default_public_args}" ${ARGN})

    if (NOT arg_OUTPUT_DIRECTORY)
        set(arg_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    endif()

    if (${arg_EXCEPTIONS})
        set(exceptions_text "EXCEPTIONS")
    endif()

    if (${arg_GUI})
        set(gui_text "GUI")
    endif()

    # Handle cases where we have a qml test without source files
    if (arg_SOURCES)
        set(private_includes
            "${CMAKE_CURRENT_SOURCE_DIR}"
            "${CMAKE_CURRENT_BINARY_DIR}"
            "$<BUILD_INTERFACE:${QT_BUILD_DIR}/include>"
             ${arg_INCLUDE_DIRECTORIES}
        )

        add_qt_executable("${name}"
            ${exceptions_text}
            ${gui_text}
            NO_INSTALL
            OUTPUT_DIRECTORY "${arg_OUTPUT_DIRECTORY}"
            SOURCES "${arg_SOURCES}"
            INCLUDE_DIRECTORIES
                ${private_includes}
            DEFINES
                QT_TESTCASE_BUILDDIR="${CMAKE_CURRENT_BINARY_DIR}"
                QT_TESTCASE_SOURCEDIR="${CMAKE_CURRENT_SOURCE_DIR}"
                ${arg_DEFINES}
            PUBLIC_LIBRARIES ${QT_CMAKE_EXPORT_NAMESPACE}::Core ${QT_CMAKE_EXPORT_NAMESPACE}::Test ${arg_PUBLIC_LIBRARIES}
            LIBRARIES ${arg_LIBRARIES}
            COMPILE_OPTIONS ${arg_COMPILE_OPTIONS}
            LINK_OPTIONS ${arg_LINK_OPTIONS}
            MOC_OPTIONS ${arg_MOC_OPTIONS}
            ENABLE_AUTOGEN_TOOLS ${arg_ENABLE_AUTOGEN_TOOLS}
            DISABLE_AUTOGEN_TOOLS ${arg_DISABLE_AUTOGEN_TOOLS}
        )

        # Tests should not be bundles on macOS even if arg_GUI is true, because some tests make
        # assumptions about the location of helper processes, and those paths would be different
        # if a test is built as a bundle.
        set_property(TARGET "${name}" PROPERTY MACOSX_BUNDLE FALSE)
        # The same goes for WIN32_EXECUTABLE, but because it will detach from the console window
        # and not print anything.
        set_property(TARGET "${name}" PROPERTY WIN32_EXECUTABLE FALSE)

        # QMLTest specifics

        extend_target("${name}" CONDITION arg_QMLTEST
            PUBLIC_LIBRARIES ${QT_CMAKE_EXPORT_NAMESPACE}::QuickTest
        )

        extend_target("${name}" CONDITION arg_QMLTEST AND NOT ANDROID
            DEFINES
                QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
        )

        extend_target("${name}" CONDITION arg_QMLTEST AND ANDROID
            DEFINES
                QUICK_TEST_SOURCE_DIR=":/"
        )
    endif()

    foreach(path IN LISTS arg_QML_IMPORTPATH)
        list(APPEND extra_test_args "-import" "${path}")
    endforeach()

    # Generate a label in the form tests/auto/foo/bar/tst_baz
    # and use it also for XML output
    file(RELATIVE_PATH label "${PROJECT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/${name}")

    if(arg_QMLTEST AND NOT arg_SOURCES)
        set(test_working_dir "${CMAKE_CURRENT_SOURCE_DIR}")
        set(test_executable ${QT_CMAKE_EXPORT_NAMESPACE}::qmltestrunner)
    else()
        set(test_working_dir "${CMAKE_CURRENT_BINARY_DIR}")
        set(test_executable "${name}")
    endif()

    add_test(NAME "${name}" COMMAND ${test_executable} ${extra_test_args} -o ${name}.xml,xml -o -,txt  WORKING_DIRECTORY "${test_working_dir}")
    set_tests_properties("${name}" PROPERTIES RUN_SERIAL "${arg_RUN_SERIAL}" LABELS "${label}")

    # Get path to qtbase/bin, then prepend this path containing the shared libraries to PATH
    set(INSTALL_PREFIX_BIN "${CMAKE_INSTALL_PREFIX}/bin")
    set_property(TEST "${name}" APPEND PROPERTY ENVIRONMENT "PATH=${CMAKE_CURRENT_BINARY_DIR}${QT_PATH_SEPARATOR}${INSTALL_PREFIX_BIN}${QT_PATH_SEPARATOR}$ENV{PATH}")

    # Add the install prefix to list of plugin paths when doing a prefix build
    if(NOT QT_INSTALL_DIR)
        list(APPEND plugin_paths "${CMAKE_INSTALL_PREFIX}/${INSTALL_PLUGINSDIR}")
    endif()
    #TODO: Collect all paths from known repositories when performing a super
    # build.
    list(APPEND plugin_paths "${PROJECT_BINARY_DIR}/${INSTALL_PLUGINSDIR}")
    list(JOIN plugin_paths "${QT_PATH_SEPARATOR}" plugin_paths_joined)
    set_property(TEST "${name}" APPEND PROPERTY ENVIRONMENT "QT_PLUGIN_PATH=${plugin_paths_joined}")

    if(ANDROID OR IOS OR WINRT)
        set(builtin_testdata TRUE)
    endif()

    if(builtin_testdata)
        # Safe guard against qml only tests, no source files == no target
        if (TARGET "${name}")
            target_compile_definitions("${name}" PRIVATE BUILTIN_TESTDATA)

            foreach(testdata IN LISTS arg_TESTDATA)
                list(APPEND builtin_files ${testdata})
            endforeach()

            set(blacklist_path "BLACKLIST")
            if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${blacklist_path}")
                list(APPEND builtin_files ${blacklist_path})
            endif()

            list(REMOVE_DUPLICATES builtin_files)

            # Skip Qt quick compiler when embedding test resources
            foreach(file IN LISTS builtin_files)
                set_source_files_properties(${file}
                    PROPERTIES QT_SKIP_QUICKCOMPILER TRUE
                )
            endforeach()

            if (builtin_files)
                add_qt_resource(${name} "${name}_testdata_builtin"
                    PREFIX "/"
                    FILES ${builtin_files}
                    BASE ${CMAKE_CURRENT_SOURCE_DIR})
            endif()
        endif()
    else()
        # Install test data
        file(RELATIVE_PATH relative_path_to_test_project
            "${QT_TOP_LEVEL_SOURCE_DIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}")
        qt_path_join(testdata_install_dir ${QT_INSTALL_DIR}
                     "${relative_path_to_test_project}")
        foreach(testdata IN LISTS arg_TESTDATA)
            set(testdata "${CMAKE_CURRENT_SOURCE_DIR}/${testdata}")
            if (IS_DIRECTORY "${testdata}")
                qt_copy_or_install(
                    DIRECTORY "${testdata}"
                    DESTINATION "${testdata_install_dir}")
            else()
                qt_copy_or_install(
                    FILES "${testdata}"
                    DESTINATION "${testdata_install_dir}")
            endif()
        endforeach()
    endif()

endfunction()


# This function creates an executable for use as a helper program with tests. Some
# tests launch separate programs to test certain input/output behavior.
# Specify OVERRIDE_OUTPUT_DIRECTORY if you dont' want to place the helper in the parent directory,
# in which case you should specify OUTPUT_DIRECTORY "/foo/bar" manually.
function(add_qt_test_helper name)

    set(add_qt_test_helper_optional_args
        "OVERRIDE_OUTPUT_DIRECTORY"
    )

    qt_parse_all_arguments(arg "add_qt_test_helper"
        "${add_qt_test_helper_optional_args};${__add_qt_executable_optional_args}"
        "${__add_qt_executable_single_args}"
        "${__add_qt_executable_multi_args}"
         ${ARGN})

    qt_remove_args(forward_args
        ARGS_TO_REMOVE
            "${name}"
            ${add_qt_test_helper_optional_args}
        ALL_ARGS
            ${add_qt_test_helper_optional_args}
            ${__add_qt_executable_optional_args}
            ${__add_qt_executable_single_args}
            ${__add_qt_executable_multi_args}
        ARGS
            ${ARGV}
    )

    set(extra_args_to_pass)
    if(NOT arg_OVERRIDE_OUTPUT_DIRECTORY)
        set(extra_args_to_pass OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/..")
    endif()

    add_qt_executable("${name}" NO_INSTALL ${extra_args_to_pass} ${forward_args})
endfunction()

# Sets QT_WILL_BUILD_TOOLS if tools will be built.
function(qt_check_if_tools_will_be_built)
    if(NOT CMAKE_CROSSCOMPILING AND NOT QT_FORCE_FIND_TOOLS)
        set(will_build_tools TRUE)
    else()
        set(will_build_tools FALSE)
    endif()
    set(QT_WILL_BUILD_TOOLS ${will_build_tools} CACHE INTERNAL "Are tools going to be built" FORCE)
endfunction()

# Wrapper function to create a regular cmake target and forward all the
# arguments collected by the conversion script. This is only meant for tests!
function(add_cmake_library target)
    # Process arguments:
    qt_parse_all_arguments(arg "add_cmake_library"
        "SHARED;MODULE;STATIC;INTERFACE"
        "OUTPUT_DIRECTORY;ARCHIVE_INSTALL_DIRECTORY;INSTALL_DIRECTORY"
        "${__default_private_args};${__default_public_args}"
        ${ARGN}
    )

    ### Define Targets:
    if(${arg_INTERFACE})
        add_library("${target}" INTERFACE)
    elseif(${arg_STATIC})
        add_library("${target}" STATIC)
    elseif(${arg_SHARED})
        add_library("${target}" SHARED)
    else()
        add_library("${target}")
    endif()

    if (NOT arg_ARCHIVE_INSTALL_DIRECTORY AND arg_INSTALL_DIRECTORY)
        set(arg_ARCHIVE_INSTALL_DIRECTORY "${arg_INSTALL_DIRECTORY}")
    endif()

    if (ANDROID)
        qt_android_apply_arch_suffix("${target}")
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

    extend_target("${target}"
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

endfunction()

# This function is used to define a "Qt tool", such as moc, uic or rcc.
# The BOOTSTRAP option allows building it as standalone program, otherwise
# it will be linked against QtCore.
function(add_qt_tool name)
    qt_parse_all_arguments(arg "add_qt_tool" "BOOTSTRAP;NO_QT;NO_INSTALL" "TOOLS_TARGET"
                               "${__default_private_args}" ${ARGN})

    # Handle case when a tool does not belong to a module and it can't be built either (like
    # during a cross-compile).
    if(NOT arg_TOOLS_TARGET AND NOT QT_WILL_BUILD_TOOLS)
        message(FATAL_ERROR "The tool \"${name}\" has not been assigned to a module via"
                            " TOOLS_TARGET (so it can't be found) and it can't be built"
                            " (QT_WILL_BUILD_TOOLS is ${QT_WILL_BUILD_TOOLS}).")
    endif()

    set(full_name "${QT_CMAKE_EXPORT_NAMESPACE}::${name}")
    if(TARGET ${full_name})
        get_property(path TARGET ${full_name} PROPERTY LOCATION)
        message(STATUS "Tool '${full_name}' was found at ${path}.")
        return()
    endif()

    if(arg_TOOLS_TARGET AND NOT QT_WILL_BUILD_TOOLS)
        set(tools_package_name "Qt6${arg_TOOLS_TARGET}Tools")
        message(STATUS "Searching for tool '${full_name}' in package ${tools_package_name}.")

        # Only search in path provided by QT_HOST_PATH. We need to do it with CMAKE_PREFIX_PATH
        # instead of PATHS option, because any find_dependency call inside a Tools package would
        # not get the proper prefix when using PATHS.
        set(BACKUP_CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH})
        set(CMAKE_PREFIX_PATH "${QT_HOST_PATH}")

        # Search both with sysroots prepended as well as in the host system. When cross compiling
        # the mode_package might be set to ONLY only, and the Qt6 tools packages are actually
        # in the host system.
        set(BACKUP_CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ${CMAKE_FIND_ROOT_PATH_MODE_PACKAGE})
        set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE "BOTH")
        set(BACKUP_CMAKE_SIZEOF_VOID_P "${CMAKE_SIZEOF_VOID_P}")
        set(CMAKE_SIZEOF_VOID_P "")
        find_package(
            ${tools_package_name}
            ${PROJECT_VERSION}
            NO_PACKAGE_ROOT_PATH
            NO_CMAKE_ENVIRONMENT_PATH
            NO_SYSTEM_ENVIRONMENT_PATH
            NO_CMAKE_PACKAGE_REGISTRY
            NO_CMAKE_SYSTEM_PATH
            NO_CMAKE_SYSTEM_PACKAGE_REGISTRY)
        set(CMAKE_SIZEOF_VOID_P "${BACKUP_CMAKE_SIZEOF_VOID_P}")
        set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE "${BACKUP_CMAKE_FIND_ROOT_PATH_MODE_PACKAGE}")
        set(CMAKE_PREFIX_PATH "${BACKUP_CMAKE_PREFIX_PATH}")

        if(${${tools_package_name}_FOUND} AND TARGET ${full_name})
            # Even if the tool is already visible, make sure that our modules remain associated
            # with the tools.
            qt_internal_append_known_modules_with_tools("${arg_TOOLS_TARGET}")
            get_property(path TARGET ${full_name} PROPERTY LOCATION)
            message(STATUS "${full_name} was found at ${path} using package ${tools_package_name}.")
            return()
        endif()
    endif()

    if(NOT QT_WILL_BUILD_TOOLS)
        message(FATAL_ERROR "The tool \"${full_name}\" was not found in the "
                           "${tools_package_name} package. "
                           "Package found: ${${tools_package_name}_FOUND}")
    else()
        message(STATUS "Tool '${full_name}' will be built from source.")
    endif()

    set(disable_autogen_tools "${arg_DISABLE_AUTOGEN_TOOLS}")
    if (arg_NO_QT)
        # FIXME: Remove NO_QT again once qmake can use a "normal" Qt!
        if (arg_BOOTSTRAP)
            message(FATAL_ERROR "Tool can not be NO_QT and BOOTSTRAP at the same time!")
        endif()
        set(corelib "")
    else()
        if (arg_BOOTSTRAP)
            set(corelib ${QT_CMAKE_EXPORT_NAMESPACE}::Bootstrap)
            list(APPEND disable_autogen_tools "uic" "moc" "rcc")
        else()
            set(corelib ${QT_CMAKE_EXPORT_NAMESPACE}::Core)
        endif()
    endif()

    set(bootstrap "")
    if(arg_BOOTSTRAP)
        set(bootstrap BOOTSTRAP)
    endif()

    set(no_qt "")
    if(arg_NO_QT)
        set(no_qt NO_QT)
    endif()

    set(no_install "")
    if(arg_NO_INSTALL)
        set(no_install NO_INSTALL)
    endif()

    add_qt_executable("${name}" OUTPUT_DIRECTORY "${QT_BUILD_DIR}/${INSTALL_BINDIR}"
        ${bootstrap}
        ${no_qt}
        ${no_install}
        SOURCES ${arg_SOURCES}
        INCLUDE_DIRECTORIES
            ${arg_INCLUDE_DIRECTORIES}
        DEFINES ${arg_DEFINES}
        PUBLIC_LIBRARIES ${corelib}
        LIBRARIES ${arg_LIBRARIES} Qt::PlatformToolInternal
        COMPILE_OPTIONS ${arg_COMPILE_OPTIONS}
        LINK_OPTIONS ${arg_LINK_OPTIONS}
        MOC_OPTIONS ${arg_MOC_OPTIONS}
        DISABLE_AUTOGEN_TOOLS ${disable_autogen_tools}
    )
    qt_internal_add_target_aliases("${name}")

    set_target_properties("${name}" PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${QT_BUILD_DIR}/${INSTALL_BINDIR}"
    )

    if(NOT arg_NO_INSTALL AND arg_TOOLS_TARGET)
        # Assign a tool to an export set, and mark the module to which the tool belongs.
        qt_internal_append_known_modules_with_tools("${arg_TOOLS_TARGET}")

        # Also append the tool to the module list.
        qt_internal_append_known_module_tool("${arg_TOOLS_TARGET}" "${name}")

        qt_install(TARGETS "${name}"
                   EXPORT "${INSTALL_CMAKE_NAMESPACE}${arg_TOOLS_TARGET}ToolsTargets"
                   DESTINATION ${INSTALL_TARGETS_DEFAULT_ARGS})
    endif()
endfunction()

function(qt_create_tracepoints name tracePointsFile)
    #### TODO
    string(TOLOWER "${name}" name)

    file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/qt${name}_tracepoints_p.h" CONTENT
        "#include <private/qtrace_p.h>")
endfunction()

# Handle files that need special SIMD-related flags.
# This creates an object library and makes target link
# to it (privately).
function(add_qt_simd_part target)
    qt_parse_all_arguments(arg "add_qt_simd_part" "" ""
       "NAME;SIMD;${__default_private_args};COMPILE_FLAGS" ${ARGN})
    if ("x${arg_SIMD}" STREQUAL x)
        message(FATAL_ERROR "add_qt_simd_part needs a SIMD type to be set.")
    endif()

    set(condition "QT_FEATURE_${arg_SIMD}")
    if("${arg_SIMD}" STREQUAL arch_haswell)
        set(condition "TEST_subarch_avx2 AND TEST_subarch_bmi AND TEST_subarch_bmi2 AND TEST_subarch_f16c AND TEST_subarch_fma AND TEST_subarch_lzcnt AND TEST_subarch_popcnt")
    elseif("${arg_SIMD}" STREQUAL avx512common)
        set(condition "TEST_subarch_avx512cd")
    elseif("${arg_SIMD}" STREQUAL avx512core)
        set(condition "TEST_subarch_avx512cd AND TEST_subarch_avx512bw AND TEST_subarch_avx512dq AND TEST_subarch_avx512vl")
    endif()

    set(name "${arg_NAME}")
    if("x${name}" STREQUAL x)
        set(name "${target}_simd_${arg_SIMD}")
    endif()

    qt_evaluate_config_expression(result ${condition})
    if(${result})
        if(QT_CMAKE_DEBUG_EXTEND_TARGET)
            message("add_qt_simd_part(${target} SIMD ${arg_SIMD} ...): Evaluated")
        endif()
        string(TOUPPER "QT_CFLAGS_${arg_SIMD}" simd_flags)

        add_library("${name}" OBJECT)
        target_sources("${name}" PRIVATE ${arg_SOURCES})
        target_include_directories("${name}" PRIVATE
            ${arg_INCLUDE_DIRECTORIES}
            $<TARGET_PROPERTY:${target},INCLUDE_DIRECTORIES>)
        target_compile_options("${name}" PRIVATE
            ${${simd_flags}}
            ${arg_COMPILE_FLAGS}
            $<TARGET_PROPERTY:${target},COMPILE_OPTIONS>)
            target_compile_definitions("${name}" PRIVATE
            $<TARGET_PROPERTY:${target},COMPILE_DEFINITIONS>)

        target_link_libraries("${target}" PRIVATE "${name}")

        if(NOT BUILD_SHARED_LIBS)
            qt_install(
              TARGETS ${name}
              EXPORT "${INSTALL_CMAKE_NAMESPACE}Targets"
            )
        endif()
    else()
        if(QT_CMAKE_DEBUG_EXTEND_TARGET)
            message("add_qt_simd_part(${target} SIMD ${arg_SIMD} ...): Skipped")
        endif()
    endif()
endfunction()

# From Qt6CoreMacros
# Function used to create the names of output files preserving relative dirs
function(qt_make_output_file infile prefix suffix source_dir binary_dir result)
    get_filename_component(outfilename "${infile}" NAME_WE)

    set(base_dir "${source_dir}")
    string(FIND "${infile}" "${binary_dir}/" in_binary)
    if (in_binary EQUAL 0)
        set(base_dir "${binary_dir}")
    endif()

    get_filename_component(abs_infile "${infile}" ABSOLUTE BASE_DIR "${base_dir}")
    file(RELATIVE_PATH rel_infile "${base_dir}" "${abs_infile}")
    string(REPLACE "../" "__/" mapped_infile "${rel_infile}")

    get_filename_component(abs_mapped_infile "${mapped_infile}" ABSOLUTE BASE_DIR "${binary_dir}")
    get_filename_component(outpath "${abs_mapped_infile}" PATH)

    file(MAKE_DIRECTORY "${outpath}")
    set("${result}" "${outpath}/${prefix}${outfilename}${suffix}" PARENT_SCOPE)
endfunction()


# Complete manual moc invocation with full control.
# Use AUTOMOC whenever possible.
function(qt_manual_moc result)
    cmake_parse_arguments(arg "" "" "FLAGS" ${ARGN})
    set(moc_files)
    foreach(infile ${arg_UNPARSED_ARGUMENTS})
        qt_make_output_file("${infile}" "moc_" ".cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}" outfile)
        list(APPEND moc_files "${outfile}")

        set(moc_parameters_file "${outfile}_parameters$<$<BOOL:$<CONFIGURATION>>:_$<CONFIGURATION>>")
        set(moc_parameters ${arg_FLAGS} -o "${outfile}" "${infile}")
        string (REPLACE ";" "\n" moc_parameters "${moc_parameters}")

        file(GENERATE OUTPUT "${moc_parameters_file}" CONTENT "${moc_parameters}\n")

        add_custom_command(OUTPUT "${outfile}"
                           COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::moc "@${moc_parameters_file}"
                           DEPENDS "${infile}" ${moc_depends} ${QT_CMAKE_EXPORT_NAMESPACE}::moc
                           WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" VERBATIM)
    endforeach()
    set("${result}" ${moc_files} PARENT_SCOPE)
endfunction()


# helper to set up a qdbusxml2cpp rule
function(qt_create_qdbusxml2cpp_command target infile)
    qt_parse_all_arguments(arg "qt_create_qdbusxml2cpp_command" "ADAPTOR;INTERFACE" "BASENAME" "FLAGS" ${ARGN})
    if((arg_ADAPTOR AND arg_INTERFACE) OR (NOT arg_ADAPTOR AND NOT arg_INTERFACE))
        message(FATAL_ERROR "qt_create_dbusxml2cpp_command needs either ADAPTOR or INTERFACE.")
    endif()

    set(option "-a")
    set(type "adaptor")
    if (arg_INTERFACE)
        set(option "-p")
        set(type "interface")
    endif()

    if ("${arg_BASENAME}" STREQUAL "")
        get_filename_component(file_dir "${infile}" DIRECTORY)
        get_filename_component(file_name "${infile}" NAME_WLE)
        get_filename_component(file_ext "${infile}" LAST_EXT)

        if("${file_ext}" STREQUAL ".xml")
        else()
            message(FATAL_ERROR "DBUS ${type} input file is not xml.")
        endif()

        # use last part of io.qt.something.xml!
        get_filename_component(file_ext "${file_name}" LAST_EXT)
        if("x${file_ext}" STREQUAL "x")
        else()
            string(SUBSTRING "${file_ext}" 1 -1 file_name) # cut of leading '.'
        endif()

        string(TOLOWER "${file_name}" file_name)
        set(file_name "${file_name}_${type}")
    else()
        set(file_name ${arg_BASENAME})
    endif()

    # Use absolute file path for the source file and set the current working directory to the
    # current binary directory, because setting an absolute path for the header:source combo option
    # does not work. Splitting on ":" breaks inside the dbus tool when running on Windows
    # due to ":" being contained in the drive path (e.g C:\foo.h:C:\foo.cpp).
    get_filename_component(absolute_in_file_path "${infile}" ABSOLUTE)

    set(header_file "${file_name}.h")
    set(source_file "${file_name}.cpp")

    add_custom_command(OUTPUT "${header_file}" "${source_file}"
                       COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::qdbusxml2cpp ${arg_FLAGS} "${option}"
                               "${header_file}:${source_file}" "${absolute_in_file_path}"
                       DEPENDS "${absolute_in_file_path}"
                       WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
                       VERBATIM)

    target_sources("${target}" PRIVATE "${header_file}" "${source_file}")
endfunction()

function(qt_compute_injection_forwarding_header target)
    qt_parse_all_arguments(arg "qt_compute_injection_forwarding_header"
                           "PRIVATE" "SOURCE;OUT_VAR" "" ${ARGN})
    qt_internal_module_info(module "${target}")
    get_filename_component(file_name "${arg_SOURCE}" NAME)

    set(source_absolute_path "${CMAKE_CURRENT_BINARY_DIR}/${arg_SOURCE}")
    file(RELATIVE_PATH relpath "${CMAKE_BINARY_DIR}" "${source_absolute_path}")

    if (arg_PRIVATE)
        set(fwd "${PROJECT_VERSION}/${module}/private/${file_name}")
    else()
        set(fwd "${file_name}")
    endif()

    string(APPEND ${arg_OUT_VAR} " ${relpath}:${fwd}")
    set(${arg_OUT_VAR} ${${arg_OUT_VAR}} PARENT_SCOPE)
endfunction()


function(add_qt_docs)
    if(${ARGC} EQUAL 1)
        # Function called from old generated CMakeLists.txt that was missing the target parameter
        return()
    endif()
    if(NOT ${ARGC} EQUAL 2)
        message(FATAL_ERROR "add_qt_docs called with the wrong number of arguments. Should be add_qt_docs(target path_to_project.qdocconf).")
        return()
    endif()
    set(target ${ARGV0})
    set(doc_project ${ARGV1})
    # TODO
endfunction()

macro(qt_find_package)
    # Get the target names we expect to be provided by the package.
    set(options CONFIG NO_MODULE MODULE REQUIRED)
    set(oneValueArgs)
    set(multiValueArgs PROVIDED_TARGETS COMPONENTS)
    cmake_parse_arguments(arg "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # If some Qt internal project calls qt_find_package(WrapFreeType), but WrapFreeType was already
    # found as part of a find_dependency() call from a ModuleDependencies.cmake file (or similar),
    # and the provided target is also found, that means this might have been an unnecessary
    # qt_find_package() call, because the dependency was already found via some other transitive
    # dependency. Return early, so that CMake doesn't fail wiht an error with trying to promote the
    # targets to be global. This behavior is not enabled by default, because there are cases
    # when a regular find_package() (non qt_) can find a package (Freetype -> PNG), and a subsequent
    # qt_find_package(PNG PROVIDED_TARGET PNG::PNG) still needs to succeed and register the provided
    # targets. To enable the debugging behavior, set QT_DEBUG_QT_FIND_PACKAGE to 1.
    set(_qt_find_package_skip_find_package FALSE)
    if(QT_DEBUG_QT_FIND_PACKAGE AND ${ARGV0}_FOUND AND arg_PROVIDED_TARGETS)
        set(_qt_find_package_skip_find_package TRUE)
        foreach(qt_find_package_target_name ${arg_PROVIDED_TARGETS})
            if(NOT TARGET ${qt_find_package_target_name})
                set(_qt_find_package_skip_find_package FALSE)
            endif()
        endforeach()

        if(_qt_find_package_skip_find_package)
            message(AUTHOR_WARNING "qt_find_package(${ARGV0}) called even though the package "
                   "was already found. Consider removing the call.")
        endif()
    endif()

    # Get the version if specified.
    set(package_version "")
    if(${ARGC} GREATER_EQUAL 2)
        if(${ARGV1} MATCHES "^[0-9\.]+$")
            set(package_version "${ARGV1}")
        endif()
    endif()

    if(arg_COMPONENTS)
        # Re-append components to forward them.
        list(APPEND arg_UNPARSED_ARGUMENTS "COMPONENTS;${arg_COMPONENTS}")
    endif()

    if(NOT (arg_CONFIG OR arg_NO_MODULE OR arg_MODULE) AND NOT _qt_find_package_skip_find_package)
        # Try to find a config package first in quiet mode
        set(config_package_arg ${arg_UNPARSED_ARGUMENTS})
        list(APPEND config_package_arg "CONFIG;QUIET")
        find_package(${config_package_arg})

        # Double check that in config mode the targets become visible. Sometimes
        # only the module mode creates the targets. For example with vcpkg, the sqlite
        # package provides sqlite3-config.cmake, which offers multi-config targets but
        # in their own way. CMake has FindSQLite3.cmake and with the original
        # qt_find_package(SQLite3) call it is our intention to use the cmake package
        # in module mode.
        if (${ARGV0}_FOUND AND arg_PROVIDED_TARGETS)
            unset(any_target_found)
            foreach(expected_target ${arg_PROVIDED_TARGETS})
                if (TARGET ${expected_target})
                    set(any_target_found TRUE)
                    break()
                endif()
            endforeach()
            if(NOT any_target_found)
                unset(${ARGV0}_FOUND)
            endif()
        endif()
    endif()

    # Ensure the options are back in the original unparsed arguments
    foreach(opt IN LISTS options)
        if(arg_${opt})
            list(APPEND arg_UNPARSED_ARGUMENTS ${opt})
        endif()
    endforeach()

    if (NOT ${ARGV0}_FOUND AND NOT _qt_find_package_skip_find_package)
        # Unset the NOTFOUND ${package}_DIR var that might have been set by the previous
        # find_package call, to get rid of "not found" messagees in the feature summary
        # if the package is found by the next find_package call.
        if(DEFINED CACHE{${ARGV0}_DIR} AND NOT ${ARGV0}_DIR)
            unset(${ARGV0}_DIR CACHE)
        endif()

        # Call original function without our custom arguments.
        find_package(${arg_UNPARSED_ARGUMENTS})
    endif()

    if(${ARGV0}_FOUND AND arg_PROVIDED_TARGETS AND NOT _qt_find_package_skip_find_package)
        # If package was found, associate each target with its package name. This will be used
        # later when creating Config files for Qt libraries, to generate correct find_dependency()
        # calls. Also make the provided targets global, so that the properties can be read in
        # all scopes.
        foreach(qt_find_package_target_name ${arg_PROVIDED_TARGETS})
            if(TARGET ${qt_find_package_target_name})
                # Allow usage of aliased targets by setting properties on the actual target
                get_target_property(aliased_target ${qt_find_package_target_name} ALIASED_TARGET)
                if(aliased_target)
                    set(qt_find_package_target_name ${aliased_target})
                endif()

                set_target_properties(${qt_find_package_target_name}
                                      PROPERTIES INTERFACE_QT_PACKAGE_NAME ${ARGV0})
                if(package_version)
                    set_target_properties(${qt_find_package_target_name}
                                          PROPERTIES INTERFACE_QT_PACKAGE_VERSION ${ARGV1})
                endif()

                if(arg_COMPONENTS)
                    string(REPLACE ";" " " components_as_string "${arg_COMPONENTS}")
                    set_property(TARGET ${qt_find_package_target_name}
                                 PROPERTY INTERFACE_QT_PACKAGE_COMPONENTS ${components_as_string})
                endif()

                get_property(is_global TARGET ${qt_find_package_target_name} PROPERTY
                                                                             IMPORTED_GLOBAL)
                if(NOT is_global)
                    set_property(TARGET ${qt_find_package_target_name} PROPERTY
                                                                       IMPORTED_GLOBAL TRUE)
                endif()
            endif()

        endforeach()
    endif()
endmacro()

# Creates a simple export set for the various Find* dependencies
# which are needed when creating a static build of Qt.
# This introduces a custom target property: INTERFACE_QT_EXPORTED_LIBRARY
# This target property indicates that Qt modules / plugins using this 3rd party library
# must add it to their list of dependencies when creating their own ${qtmodule}Config.cmake
function(qt_install_static_target_export target)
    if(BUILD_SHARED_LIBS)
        return()
    endif()

    qt_parse_all_arguments(arg "qt_install_3rdparty_config_files" "" "EXPORT" "" ${ARGN})
    # TODO mark EXPORT as required

    set_target_properties(${target}
        PROPERTIES
            INTERFACE_QT_EXPORTED_LIBRARY 1)

    qt_path_join(config_install_dir ${QT_CONFIG_INSTALL_DIR} ${arg_EXPORT})

    set(export_name "${arg_EXPORT}Targets")
    qt_install(
        TARGETS ${target}
        EXPORT ${export_name}
        LIBRARY DESTINATION ${INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${INSTALL_LIBDIR})

    qt_install(
        EXPORT ${export_name}
        DESTINATION "${config_install_dir}"
    )
endfunction()

# Create a set of ${target}Config.cmake and ${target}Version.cmake for a
# third-party library so that it can be found by client code linking statically.
function(qt_install_3rdparty_config_files target)
    if(BUILD_SHARED_LIBS)
       return()
    endif()

    qt_parse_all_arguments(arg "qt_install_3rdparty_config_files" "" "EXPORT" "PACKAGES;ADDITIONAL_FILES" ${ARGN})
    # TODO mark EXPORT as required

    set(3RDPARTY_ADDITIONAL_SETUP_CODE)
    foreach(package ${arg_PACKAGES})
      list(APPEND 3RDPARTY_ADDITIONAL_SETUP_CODE "find_package(${package})\n")
    endforeach()

    set(path_suffix "${arg_EXPORT}")
    qt_path_join(config_build_dir ${QT_CONFIG_BUILD_DIR} ${path_suffix})
    qt_path_join(config_install_dir ${QT_CONFIG_INSTALL_DIR} ${path_suffix})

    configure_package_config_file(
        "${PROJECT_SOURCE_DIR}/cmake/3rdpartyConfig.cmake.in"
        "${config_build_dir}/${target}Config.cmake"
        INSTALL_DESTINATION "${config_install_dir}"
    )

    write_basic_package_version_file(
        "${config_build_dir}/${target}ConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
    )

    qt_install(FILES
        "${config_build_dir}/${target}Config.cmake"
        "${config_build_dir}/${target}ConfigVersion.cmake"
        ${arg_ADDITIONAL_FILES}
        DESTINATION "${config_install_dir}"
        COMPONENT Devel
    )
endfunction()

# Call this function in 3rdparty find modules that ought to be installed alongside
# Qt modules and must be found when linking statically.
function(qt_install_3rdparty_library target)
    qt_install_static_target_export(${target} EXPORT ${target})
    qt_install_3rdparty_config_files(${target} EXPORT ${target} ${ARGN})
endfunction()

macro(qt_find_apple_system_frameworks)
    if(APPLE)
        find_library(FWAppKit AppKit)
        find_library(FWApplicationServices ApplicationServices)
        find_library(FWCarbon Carbon)
        find_library(FWCoreFoundation CoreFoundation)
        find_library(FWCoreServices CoreServices)
        find_library(FWCoreVideo CoreVideo)
        find_library(FWcups cups)
        find_library(FWDiskArbitration DiskArbitration)
        find_library(FWFoundation Foundation)
        find_library(FWIOBluetooth IOBluetooth)
        find_library(FWIOKit IOKit)
        find_library(FWIOSurface IOSurface)
        find_library(FWImageIO ImageIO)
        find_library(FWMetal Metal)
        find_library(FWMobileCoreServices MobileCoreServices)
        find_library(FWQuartzCore QuartzCore)
        find_library(FWSecurity Security)
        find_library(FWSystemConfiguration SystemConfiguration)
        find_library(FWUIKit UIKit)
        find_library(FWWatchKit WatchKit)
        find_library(FWGameController GameController)
    endif()
endmacro()

# Match the pattern 'regex' in 'input_line', replace the match with 'replacement'
# and set that result in 'out_var' in the parent scope.
function(qt_regex_match_and_get input_line regex replacement out_var)
    string(REGEX MATCH "${regex}" match "${input_line}")
    if(match)
        string(REGEX REPLACE "${regex}" "${replacement}" match "${input_line}")
        string(STRIP ${match} match)
        set(${out_var} "${match}" PARENT_SCOPE)
    endif()
endfunction()

# Match 'regex' in a list of lines. When found, set the value to 'out_var' and break early.
function(qt_qlalr_find_option_in_list input_list regex out_var)
    foreach(line ${input_list})
        qt_regex_match_and_get("${line}" "${regex}" "\\1" option)
        if(option)
            string(TOLOWER ${option} option)
            set(${out_var} "${option}" PARENT_SCOPE)
            return()
        endif()
    endforeach()
    message(FATAL_ERROR "qt_qlalr_find_option_in_list: Could not extract ${out_var}")
endfunction()

# Generate a few output files using qlalr, and assign those to 'consuming_target'.
# 'input_file_list' is a list of 'foo.g' file paths.
# 'flags' are extra flags to be passed to qlalr.
function(qt_process_qlalr consuming_target input_file_list flags)
    foreach(input_file ${input_file_list})
        file(STRINGS ${input_file} input_file_lines)
        qt_qlalr_find_option_in_list("${input_file_lines}" "^%parser(.+)" "parser")
        qt_qlalr_find_option_in_list("${input_file_lines}" "^%decl(.+)" "decl")
        qt_qlalr_find_option_in_list("${input_file_lines}" "^%impl(.+)" "impl")
        get_filename_component(base_file_name ${input_file} NAME_WE)

        set(cpp_file "${parser}.cpp")
        set(private_file "${parser}_p.h")
        set(decl_file "${decl}")
        set(impl_file "${impl}")
        add_custom_command(
            OUTPUT ${cpp_file} ${private_file} ${decl_file} ${impl_file}
            COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::qlalr ${flags} ${input_file}
            MAIN_DEPENDENCY ${input_file}
        )
        target_sources(${consuming_target} PRIVATE ${cpp_file} ${impl_file})
    endforeach()
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

    # Start first part.
    qt_add_string_to_qconfig_cpp("doc")
    qt_add_string_to_qconfig_cpp("include")
    qt_add_string_to_qconfig_cpp("lib")
    qt_add_string_to_qconfig_cpp("libexec")
    qt_add_string_to_qconfig_cpp("bin")
    qt_add_string_to_qconfig_cpp("plugins")
    qt_add_string_to_qconfig_cpp("imports")
    qt_add_string_to_qconfig_cpp("qml")
    qt_add_string_to_qconfig_cpp(".")
    qt_add_string_to_qconfig_cpp(".")
    qt_add_string_to_qconfig_cpp("translations")
    qt_add_string_to_qconfig_cpp("examples")
    qt_add_string_to_qconfig_cpp("tests")

    # Save first part.
    set(QT_CONFIG_STR_OFFSETS_FIRST "${QT_CONFIG_STR_OFFSETS}")
    set(QT_CONFIG_STRS_FIRST "${QT_CONFIG_STRS}")

    # Start second part.
    set(QT_CONFIG_STR_OFFSETS "")
    set(QT_CONFIG_STRS "")

    qt_add_string_to_qconfig_cpp("")
    qt_add_string_to_qconfig_cpp("false")
    qt_add_string_to_qconfig_cpp("bin")
    qt_add_string_to_qconfig_cpp("lib")
    qt_add_string_to_qconfig_cpp(".")
    qt_add_string_to_qconfig_cpp("${QT_QMAKE_TARGET_MKSPEC}")
    qt_add_string_to_qconfig_cpp("${QT_QMAKE_HOST_MKSPEC}")

    # Save second part.
    set(QT_CONFIG_STR_OFFSETS_SECOND "${QT_CONFIG_STR_OFFSETS}")
    set(QT_CONFIG_STRS_SECOND "${QT_CONFIG_STRS}")

    # Settings path / sysconf dir.
    if(APPLE)
        set(QT_DEFAULT_SYS_CONF_DIR "/Library/Preferences/Qt")
    else()
        set(QT_DEFAULT_SYS_CONF_DIR "etc/xdg")
    endif()

    # Compute and set relocation prefixes.
    # TODO: Clean this up, there's a bunch of unrealistic assumptions here.
    # See qtConfOutput_preparePaths in qtbase/configure.pri.
    if(WIN32)
        set(lib_location_absolute_path "${CMAKE_INSTALL_PREFIX}/bin")
    else()
        set(lib_location_absolute_path "${CMAKE_INSTALL_PREFIX}/lib")
    endif()
    file(RELATIVE_PATH from_lib_location_to_prefix
         "${lib_location_absolute_path}" "${CMAKE_INSTALL_PREFIX}")

    if(QT_HOST_PATH)
        set(host_prefix "${QT_HOST_PATH}")
        set(host_bin_dir_absolute_path "${QT_HOST_PATH}/bin")
    else()
        set(host_prefix "${CMAKE_INSTALL_PREFIX}")
        set(host_bin_dir_absolute_path "${CMAKE_INSTALL_PREFIX}/bin")
    endif()

    file(RELATIVE_PATH from_host_bin_dir_to_host_prefix
         "${host_bin_dir_absolute_path}" "${host_prefix}")

    # TODO: Fix this to use the equivalent of extprefix on CMake (CMAKE_STAGING_PREFIX?)
    # For now just assume ext prefix is same as regular prefix.
    file(RELATIVE_PATH from_host_bin_dir_to_ext_prefix
         "${host_bin_dir_absolute_path}" "${CMAKE_INSTALL_PREFIX}")


    set(QT_CONFIGURE_LIBLOCATION_TO_PREFIX_PATH "${from_lib_location_to_prefix}")
    set(QT_CONFIGURE_HOSTBINDIR_TO_HOSTPREFIX_PATH "${from_host_bin_dir_to_host_prefix}")
    set(QT_CONFIGURE_HOSTBINDIR_TO_EXTPREFIX_PATH "${from_host_bin_dir_to_ext_prefix}")

    configure_file(global/qconfig.cpp.in global/qconfig.cpp @ONLY)
endfunction()

function(qt_set_language_standards)
    ## Use the latest standard the compiler supports (same as qt_common.prf)
    if (QT_FEATURE_cxx2a)
        set(CMAKE_CXX_STANDARD 20 PARENT_SCOPE)
    elseif (QT_FEATURE_cxx17)
        set(CMAKE_CXX_STANDARD 17 PARENT_SCOPE)
    elseif (QT_FEATURE_cxx14)
        set(CMAKE_CXX_STANDARD 14 PARENT_SCOPE)
    elseif (QT_FEATURE_cxx11)
        set(CMAKE_CXX_STANDARD 11 PARENT_SCOPE)
    endif()

    if (c_std_11 IN_LIST CMAKE_C_COMPILE_FEATURES)
        set(CMAKE_C_STANDARD 11 PARENT_SCOPE)
    elseif (c_std_99 IN_LIST CMAKE_C_COMPILE_FEATURES)
        set(CMAKE_C_STANDARD 99 PARENT_SCOPE)
    endif()
endfunction()
