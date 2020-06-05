include(CMakePackageConfigHelpers)
include(QtSeparateDebugInfo)

function(qt_configure_process_path name default docstring)
    # Values are computed once for qtbase, and then exported and reused for other projects.
    if(NOT PROJECT_NAME STREQUAL "QtBase")
        return()
    endif()

    # No value provided, set the default.
    if(NOT DEFINED "${name}")
        set("${name}" "${default}" CACHE STRING "${docstring}")
    else()
        get_filename_component(given_path_as_abs "${${name}}" ABSOLUTE BASE_DIR
                               "${CMAKE_INSTALL_PREFIX}")
        file(RELATIVE_PATH rel_path "${CMAKE_INSTALL_PREFIX}"
                                    "${given_path_as_abs}")

        # If absolute path given, check that it's inside the prefix (error out if not).
        # TODO: Figure out if we need to support paths that are outside the prefix.
        #
        # If relative path given, it's relative to the install prefix (rather than the binary dir,
        # which is what qmake does for some reason).
        # In both cases, store the value as a relative path.
        if("${rel_path}" STREQUAL "")
            # file(RELATIVE_PATH) returns an empty string if the given absolute paths are equal
            set(rel_path ".")
        elseif(rel_path MATCHES "^\.\./")
            message(FATAL_ERROR
                "Path component '${name}' is outside computed install prefix: ${rel_path} ")
        endif()
        set("${name}" "${rel_path}" CACHE STRING "${docstring}" FORCE)
    endif()
endfunction()

# Install locations:
qt_configure_process_path(INSTALL_BINDIR "bin" "Executables [PREFIX/bin]")
qt_configure_process_path(INSTALL_INCLUDEDIR "include" "Header files [PREFIX/include]")
qt_configure_process_path(INSTALL_LIBDIR "lib" "Libraries [PREFIX/lib]")
qt_configure_process_path(INSTALL_MKSPECSDIR "mkspecs" "Mkspecs files [PREFIX/mkspecs]")
qt_configure_process_path(INSTALL_ARCHDATADIR "." "Arch-dependent data [PREFIX]")
qt_configure_process_path(INSTALL_PLUGINSDIR
                          "${INSTALL_ARCHDATADIR}/plugins"
                          "Plugins [ARCHDATADIR/plugins]")

# Given CMAKE_CONFIG and ALL_CMAKE_CONFIGS, determines if a directory suffix needs to be appended
# to each destination, and sets the computed install target destination arguments in OUT_VAR.
# Defaults used for each of the destination types, and can be configured per destination type.
function(qt_get_install_target_default_args)
    qt_parse_all_arguments(arg "qt_get_install_target_default_args"
                               "" "OUT_VAR;CMAKE_CONFIG;RUNTIME;LIBRARY;ARCHIVE;INCLUDES;BUNDLE"
                                  "ALL_CMAKE_CONFIGS" ${ARGN})

    if(NOT arg_CMAKE_CONFIG)
        message(FATAL_ERROR "No value given for CMAKE_CONFIG.")
    endif()
    if(NOT arg_ALL_CMAKE_CONFIGS)
        message(FATAL_ERROR "No value given for ALL_CMAKE_CONFIGS.")
    endif()
    list(LENGTH arg_ALL_CMAKE_CONFIGS all_configs_count)
    list(GET arg_ALL_CMAKE_CONFIGS 0 first_config)

    set(suffix "")
    if(all_configs_count GREATER 1 AND NOT arg_CMAKE_CONFIG STREQUAL first_config)
        set(suffix "/${arg_CMAKE_CONFIG}")
    endif()

    set(runtime "${INSTALL_BINDIR}")
    if(arg_RUNTIME)
        set(runtime "${arg_RUNTIME}")
    endif()

    set(library "${INSTALL_LIBDIR}")
    if(arg_LIBRARY)
        set(library "${arg_LIBRARY}")
    endif()

    set(archive "${INSTALL_LIBDIR}")
    if(arg_ARCHIVE)
        set(archive "${arg_ARCHIVE}")
    endif()

    set(includes "${INSTALL_INCLUDEDIR}")
    if(arg_INCLUDES)
        set(includes "${arg_INCLUDES}")
    endif()

    set(bundle "${INSTALL_BINDIR}")
    if(arg_BUNDLE)
        set(bundle "${arg_BUNDLE}")
    endif()

    set(args
        RUNTIME DESTINATION  "${runtime}${suffix}"
        LIBRARY DESTINATION  "${library}${suffix}"
        ARCHIVE DESTINATION  "${archive}${suffix}" COMPONENT Devel
        BUNDLE DESTINATION   "${bundle}${suffix}"
        INCLUDES DESTINATION "${includes}${suffix}")
    set(${arg_OUT_VAR} "${args}" PARENT_SCOPE)
endfunction()

if (WIN32)
    set(_default_libexec "${INSTALL_ARCHDATADIR}/bin")
else()
    set(_default_libexec "${INSTALL_ARCHDATADIR}/libexec")
endif()

qt_configure_process_path(
    INSTALL_LIBEXECDIR
    "${_default_libexec}"
    "Helper programs [ARCHDATADIR/bin on Windows, ARCHDATADIR/libexec otherwise]")
qt_configure_process_path(INSTALL_QMLDIR
                          "${INSTALL_ARCHDATADIR}/qml"
                           "QML2 imports [ARCHDATADIR/qml]")
qt_configure_process_path(INSTALL_DATADIR "." "Arch-independent data [PREFIX]")
qt_configure_process_path(INSTALL_DOCDIR "${INSTALL_DATADIR}/doc" "Documentation [DATADIR/doc]")
qt_configure_process_path(INSTALL_TRANSLATIONSDIR "${INSTALL_DATADIR}/translations"
    "Translations [DATADIR/translations]")
qt_configure_process_path(INSTALL_SYSCONFDIR
                          "etc/xdg"
                          "Settings used by Qt programs [PREFIX/etc/xdg]")
qt_configure_process_path(INSTALL_EXAMPLESDIR "examples" "Examples [PREFIX/examples]")
qt_configure_process_path(INSTALL_TESTSDIR "tests" "Tests [PREFIX/tests]")
qt_configure_process_path(INSTALL_DESCRIPTIONSDIR
                         "${INSTALL_DATADIR}/modules"
                          "Module description files directory")

function(qt_internal_set_up_global_paths)
    # Compute the values of QT_BUILD_DIR, QT_INSTALL_DIR, QT_CONFIG_BUILD_DIR, QT_CONFIG_INSTALL_DIR
    # taking into account whether the current build is a prefix build or a non-prefix build,
    # and whether it is a superbuild or non-superbuild.
    # A third case is when another module or standalone tests are built against a super-built Qt.
    # The layout for the third case is the same as for non-superbuilds.
    #
    # These values should be prepended to file paths in commands or properties,
    # in order to correctly place generated Config files, generated Targets files,
    # excutables / libraries, when copying / installing files, etc.
    #
    # The build dir variables will always be absolute paths.
    # The QT_INSTALL_DIR variable will have a relative path in a prefix build,
    # which means that it can be empty, so use qt_join_path to prevent accidental absolute paths.
    if(QT_SUPERBUILD)
        # In this case, we always copy all the build products in qtbase/{bin,lib,...}
        if(QT_WILL_INSTALL)
            set(QT_BUILD_DIR "${QtBase_BINARY_DIR}")
            set(QT_INSTALL_DIR "")
        else()
            set(QT_BUILD_DIR "${QtBase_BINARY_DIR}")
            set(QT_INSTALL_DIR "${QtBase_BINARY_DIR}")
        endif()
    else()
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
    endif()

    set(__config_path_part "${INSTALL_LIBDIR}/cmake")
    set(QT_CONFIG_BUILD_DIR "${QT_BUILD_DIR}/${__config_path_part}")
    set(QT_CONFIG_INSTALL_DIR "${QT_INSTALL_DIR}")
    if(QT_CONFIG_INSTALL_DIR)
        string(APPEND QT_CONFIG_INSTALL_DIR "/")
    endif()
    string(APPEND QT_CONFIG_INSTALL_DIR ${__config_path_part})

    set(QT_BUILD_DIR "${QT_BUILD_DIR}" PARENT_SCOPE)
    set(QT_INSTALL_DIR "${QT_INSTALL_DIR}" PARENT_SCOPE)
    set(QT_CONFIG_BUILD_DIR "${QT_CONFIG_BUILD_DIR}" PARENT_SCOPE)
    set(QT_CONFIG_INSTALL_DIR "${QT_CONFIG_INSTALL_DIR}" PARENT_SCOPE)
endfunction()
qt_internal_set_up_global_paths()
qt_get_relocatable_install_prefix(QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX)

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

# Find the path to mkspecs/, depending on whether we are building as part of a standard qtbuild,
# or a module against an already installed version of qt.
if(NOT QT_MKSPECS_DIR)
    if("${QT_BUILD_INTERNALS_PATH}" STREQUAL "")
      get_filename_component(QT_MKSPECS_DIR "${CMAKE_CURRENT_LIST_DIR}/../mkspecs" ABSOLUTE)
    else()
      # We can rely on QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX being set by
      # QtBuildInternalsExtra.cmake.
      get_filename_component(
          QT_MKSPECS_DIR
          "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${INSTALL_MKSPECSDIR}" ABSOLUTE)
    endif()
    set(QT_MKSPECS_DIR "${QT_MKSPECS_DIR}" CACHE INTERNAL "")
endif()

# the default RPATH to be used when installing, but only if it's not a system directory
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/${INSTALL_LIBDIR}" isSystemDir)
if("${isSystemDir}" STREQUAL "-1")
   set(_default_install_rpath "${CMAKE_INSTALL_PREFIX}/${INSTALL_LIBDIR}")
endif("${isSystemDir}" STREQUAL "-1")

# The default rpath settings for installed targets is empty.
# The rpaths will instead be computed for each target separately using qt_apply_rpaths().
# Additional rpaths can be passed via QT_EXTRA_RPATHS.
# By default this will include $ORIGIN / @loader_path, so the installation is relocatable.
# Bottom line: No need to pass anything to CMAKE_INSTALL_RPATH.
set(CMAKE_INSTALL_RPATH "" CACHE STRING "RPATH for installed binaries")

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

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

function(qt_setup_tool_path_command)
    if(NOT WIN32)
        return()
    endif()
    set(bindir "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${INSTALL_BINDIR}")
    file(TO_NATIVE_PATH "${bindir}" bindir)
    list(APPEND command COMMAND)
    list(APPEND command set \"PATH=${bindir}$<SEMICOLON>%PATH%\")
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
    elseif(CLANG AND MINGW)
        set(QT_QMAKE_TARGET_MKSPEC win32-clang-g++)
    elseif(MINGW)
        set(QT_QMAKE_TARGET_MKSPEC win32-g++)
    endif()

    if (MINGW)
        list(APPEND QT_DEFAULT_PLATFORM_DEFINITIONS _WIN32_WINNT=0x0601 MINGW_HAS_SECURE_API=1)
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

# Platform definition dir provided by user on command line.
# Derive the absolute one relative to the current source dir.
if(QT_PLATFORM_DEFINITION_DIR)
    set(QT_DEFAULT_PLATFORM_DEFINITION_DIR "${QT_PLATFORM_DEFINITION_DIR}")
    get_filename_component(
        QT_DEFAULT_PLATFORM_DEFINITION_DIR_ABSOLUTE
        "${QT_PLATFORM_DEFINITION_DIR}"
        ABSOLUTE)
elseif(QT_QMAKE_TARGET_MKSPEC)
    # Used by consumers of prefix builds via INSTALL_INTERFACE (relative path).
    set(QT_DEFAULT_PLATFORM_DEFINITION_DIR "${INSTALL_MKSPECSDIR}/${QT_QMAKE_TARGET_MKSPEC}")
    # Used by qtbase itself and consumers of non-prefix builds via BUILD_INTERFACE (absolute path).
    set(QT_DEFAULT_PLATFORM_DEFINITION_DIR_ABSOLUTE "${QT_MKSPECS_DIR}/${QT_QMAKE_TARGET_MKSPEC}")
endif()

if(NOT DEFINED QT_DEFAULT_PLATFORM_DEFINITIONS)
    set(QT_DEFAULT_PLATFORM_DEFINITIONS "")
endif()

set(QT_PLATFORM_DEFINITIONS ${QT_DEFAULT_PLATFORM_DEFINITIONS}
    CACHE STRING "Qt platform specific pre-processor defines")
set(QT_PLATFORM_DEFINITION_DIR "${QT_DEFAULT_PLATFORM_DEFINITION_DIR}"
    CACHE PATH "Path to directory that contains qplatformdefs.h")
set(QT_PLATFORM_DEFINITION_DIR_ABSOLUTE "${QT_DEFAULT_PLATFORM_DEFINITION_DIR_ABSOLUTE}"
    CACHE INTERNAL "Path to directory that contains qplatformdefs.h")
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
        set(QT_KNOWN_MODULE_${module}_TOOLS ""
            CACHE INTERNAL "Known Qt module ${module} tools" FORCE)
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
function(qt_remove_install_target)
    # On superbuilds we only do this for qtbase - it will correctly remove the
    # cmake_install.cmake at the root of the repository.
    if(QT_SUPERBUILD)
      if(NOT (PROJECT_NAME STREQUAL "QtBase"))
        return()
      endif()
    endif()

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
        qt_remove_install_target()
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

# Creates a regular expression that exactly matches the given string
# Found in https://gitlab.kitware.com/cmake/cmake/issues/18580
function(qt_re_escape out_var str)
    string(REGEX REPLACE "([][+.*()^])" "\\\\\\1" regex "${str}")
    set(${out_var} ${regex} PARENT_SCOPE)
endfunction()

# Extracts the 3rdparty libraries for the module ${module_name}
# and stores the information in cmake language in
# ${output_root_dir}/$<CONFIG>/${output_file_name}.
#
# This function "follows" INTERFACE_LIBRARY targets to "real" targets
# and collects defines, include dirs and lib dirs on the way.
function(qt_generate_qmake_libraries_pri_content module_name output_root_dir output_file_name)
    set(content "")

    # Set up a regular expression that matches all implicit include dirs
    set(implicit_include_dirs_regex "")
    foreach(dir ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
        qt_re_escape(regex "${dir}")
        list(APPEND implicit_include_dirs_regex ${regex})
    endforeach()
    list(JOIN implicit_include_dirs_regex "|" implicit_include_dirs_regex)

    foreach(lib ${QT_QMAKE_LIBS_FOR_${module_name}})
        set(lib_targets ${QT_QMAKE_LIB_TARGETS_${lib}})
        string(TOUPPER ${lib} uclib)
        set(lib_defines "")
        set(lib_incdir "")
        set(lib_libdir "")
        set(lib_libs "")
        while(lib_targets)
            list(POP_BACK lib_targets lib_target)
            if(TARGET ${lib_target})
                get_target_property(lib_target_type ${lib_target} TYPE)
                if(lib_target_type STREQUAL "INTERFACE_LIBRARY")
                    get_target_property(iface_libs ${lib_target} INTERFACE_LINK_LIBRARIES)
                    if(iface_libs)
                        list(PREPEND lib_targets ${iface_libs})
                    endif()
                else()
                    list(APPEND lib_libs "$<TARGET_LINKER_FILE:${lib_target}>")
                endif()
                list(APPEND lib_libdir  "$<TARGET_PROPERTY:${lib_target},INTERFACE_LINK_DIRECTORIES>")
                list(APPEND lib_incdir  "$<TARGET_PROPERTY:${lib_target},INTERFACE_INCLUDE_DIRECTORIES>")
                list(APPEND lib_defines "$<TARGET_PROPERTY:${lib_target},INTERFACE_COMPILE_DEFINITIONS>")
            else()
                list(APPEND lib_libs "${lib_target}")
            endif()
        endwhile()

        # Wrap in $<REMOVE_DUPLICATES:...> but not the libs, because
        # we would have to preserve the right order for the linker.
        foreach(sfx libdir incdir defines)
            string(PREPEND lib_${sfx} "$<REMOVE_DUPLICATES:")
            string(APPEND lib_${sfx} ">")
        endforeach()

        # Filter out implicit include directories
        string(PREPEND lib_incdir "$<FILTER:")
        string(APPEND lib_incdir ",EXCLUDE,${implicit_include_dirs_regex}>")

        set(uccfg $<UPPER_CASE:$<CONFIG>>)
        string(APPEND content "list(APPEND known_libs ${uclib})
set(QMAKE_LIBS_${uclib}_${uccfg} \"${lib_libs}\")
set(QMAKE_LIBDIR_${uclib}_${uccfg} \"${lib_libdir}\")
set(QMAKE_INCDIR_${uclib}_${uccfg} \"${lib_incdir}\")
set(QMAKE_DEFINES_${uclib}_${uccfg} \"${lib_defines}\")
")
        if(QT_QMAKE_LIB_DEPS_${lib})
            string(APPEND content "set(QMAKE_DEPENDS_${uclib}_CC, ${deps})
set(QMAKE_DEPENDS_${uclib}_LD, ${deps})
")
        endif()
    endforeach()

    file(GENERATE
        OUTPUT "${output_root_dir}/$<CONFIG>/${output_file_name}"
        CONTENT "${content}"
    )
endfunction()

# Retrieves the public Qt module dependencies of the given Qt module or Qt Private module.
function(qt_get_direct_module_dependencies target out_var)
    set(dependencies "")
    get_target_property(libs ${target} INTERFACE_LINK_LIBRARIES)
    if(NOT libs)
        set(libs "")
    endif()
    get_target_property(target_type ${target} TYPE)
    while(libs)
        list(POP_FRONT libs lib)
        string(GENEX_STRIP "${lib}" lib)
        if(NOT lib OR NOT TARGET "${lib}")
            continue()
        endif()
        get_target_property(lib_type ${lib} TYPE)
        get_target_property(is_versionless_target ${lib} _qt_is_versionless_target)
        if (lib_type STREQUAL "INTERFACE_LIBRARY" AND is_versionless_target)
            # Found a version-less target like Qt::Core outside of qtbase.
            # Skip this one and use what this target points to, e.g. Qt6::Core.
            # Make sure to process Private interface libraries as-is.
            get_target_property(ifacelibs ${lib} INTERFACE_LINK_LIBRARIES)
            list(PREPEND libs ${ifacelibs})
            continue()
        endif()
        if(lib_type STREQUAL "OBJECT_LIBRARY")
            # Skip object libraries, because they're already part of ${target}.
            continue()
        elseif(lib_type STREQUAL "STATIC_LIBRARY" AND target_type STREQUAL "SHARED_LIBRARY")
            # Skip static libraries if ${target} is a shared library.
            continue()
        endif()
        get_target_property(lib_config_module_name ${lib} "_qt_config_module_name")
        if(lib_config_module_name)
            list(APPEND dependencies ${lib_config_module_name})
        endif()
    endwhile()
    set(${out_var} ${dependencies} PARENT_SCOPE)
endfunction()

# Generates module .pri files for consumption by qmake
function(qt_generate_module_pri_file target)
    set(flags INTERNAL_MODULE HEADER_MODULE)
    set(options)
    set(multiopts)
    cmake_parse_arguments(arg "${flags}" "${options}" "${multiopts}" ${ARGN})

    qt_internal_module_info(module "${target}")
    set(pri_files)

    set(property_prefix)
    if(arg_HEADER_MODULE)
        set(property_prefix "INTERFACE_")
    endif()

    get_target_property(enabled_features "${target}"
                        "${property_prefix}QT_ENABLED_PUBLIC_FEATURES")
    get_target_property(disabled_features "${target}"
                        "${property_prefix}QT_DISABLED_PUBLIC_FEATURES")
    get_target_property(enabled_private_features "${target}"
                        "${property_prefix}QT_ENABLED_PRIVATE_FEATURES")
    get_target_property(disabled_private_features "${target}"
                        "${property_prefix}QT_DISABLED_PRIVATE_FEATURES")
    qt_correct_features(enabled_features "${enabled_features}")
    qt_correct_features(disabled_features "${disabled_features}")
    qt_correct_features(enabled_private_features "${enabled_private_features}")
    qt_correct_features(disabled_private_features "${disabled_private_features}")

    foreach(var enabled_features disabled_features enabled_private_features disabled_private_features)
        if(${var} STREQUAL "${var}-NOTFOUND")
            set(${var} "")
        else()
            string (REPLACE ";" " " ${var} "${${var}}")
        endif()
    endforeach()

    set(module_internal_config v2)
    if(NOT QT_FEATURE_shared)
        list(APPEND module_internal_config staticlib)
    endif()
    if(arg_INTERNAL_MODULE)
        list(APPEND module_internal_config internal_module)
    endif()

    get_target_property(target_type ${target} TYPE)
    if (NOT target_type STREQUAL "INTERFACE_LIBRARY")
        get_target_property(is_fw ${target} FRAMEWORK)
        if(is_fw)
            list(APPEND module_internal_config lib_bundle)
        endif()
    endif()

    # TODO: Add the value 'ltcg' to module_internal_config if LTCG is turned on.

    list(JOIN module_internal_config " " joined_module_internal_config)

    get_target_property(config_module_name ${target} _qt_config_module_name)
    get_target_property(qmake_module_config ${target} ${property_prefix}QT_QMAKE_MODULE_CONFIG)
    if(qmake_module_config)
        string(REPLACE ";" " " module_build_config "${qmake_module_config}")
        set(module_build_config "\nQT.${config_module_name}.CONFIG = ${module_build_config}")
    else()
        set(module_build_config "")
    endif()

    if(is_fw)
        set(framework_base_path "$$QT_MODULE_LIB_BASE/${module}.framework/Headers")
        set(public_module_includes "${framework_base_path}")
        set(public_module_frameworks "$$QT_MODULE_LIB_BASE")
        set(private_module_includes "${framework_base_path}/${PROJECT_VERSION} ${framework_base_path}/${PROJECT_VERSION}/${module}")
        set(module_name_in_pri "${module}")
    else()
        set(public_module_includes "$$QT_MODULE_INCLUDE_BASE $$QT_MODULE_INCLUDE_BASE/${module}")
        set(public_module_frameworks "")
        set(private_module_includes "$$QT_MODULE_INCLUDE_BASE/${module}/${PROJECT_VERSION} $$QT_MODULE_INCLUDE_BASE/${module}/${PROJECT_VERSION}/${module}")
        set(module_name_in_pri "${module_versioned}")
    endif()

    qt_path_join(target_path ${QT_BUILD_DIR} ${INSTALL_MKSPECSDIR}/modules)
    if (arg_INTERNAL_MODULE)
        string(PREPEND private_module_includes "${public_module_includes} ")
        set(private_module_frameworks ${public_module_frameworks})
    else()
        unset(private_module_frameworks)
        if(arg_HEADER_MODULE)
            set(module_plugin_types "")
        else()
            get_target_property(module_plugin_types ${target} MODULE_PLUGIN_TYPES)
            if(module_plugin_types)
                list(JOIN module_plugin_types " " module_plugin_types)
            else()
                set(module_plugin_types "")
            endif()
        endif()

        qt_get_direct_module_dependencies(${target} public_module_dependencies)
        list(JOIN public_module_dependencies " " public_module_dependencies)

        qt_path_join(pri_file_name "${target_path}" "qt_lib_${config_module_name}.pri")
        list(APPEND pri_files "${pri_file_name}")

        file(GENERATE
            OUTPUT "${pri_file_name}"
            CONTENT
        "QT.${config_module_name}.VERSION = ${PROJECT_VERSION}
QT.${config_module_name}.name = ${module}
QT.${config_module_name}.module = ${module_name_in_pri}
QT.${config_module_name}.libs = $$QT_MODULE_LIB_BASE
QT.${config_module_name}.includes = ${public_module_includes}
QT.${config_module_name}.frameworks = ${public_module_frameworks}
QT.${config_module_name}.bins = $$QT_MODULE_BIN_BASE
QT.${config_module_name}.plugin_types = ${module_plugin_types}
QT.${config_module_name}.depends = ${public_module_dependencies}
QT.${config_module_name}.uses =
QT.${config_module_name}.module_config = ${joined_module_internal_config}
QT.${config_module_name}.DEFINES = QT_${module_define}_LIB
QT.${config_module_name}.enabled_features = ${enabled_features}
QT.${config_module_name}.disabled_features = ${disabled_features}${module_build_config}
QT_CONFIG += ${enabled_features}
QT_MODULES += ${config_module_name}
"
        )
    endif()

    set(pri_data_cmake_file "qt_lib_${config_module_name}_private.cmake")
    qt_generate_qmake_libraries_pri_content(${config_module_name} "${CMAKE_CURRENT_BINARY_DIR}"
        ${pri_data_cmake_file})

    set(private_pri_file_name "qt_lib_${config_module_name}_private.pri")

    set(private_module_dependencies "")
    if(NOT arg_HEADER_MODULE)
        qt_get_direct_module_dependencies(${target}Private private_module_dependencies)
    endif()
    list(JOIN private_module_dependencies " " private_module_dependencies)

    # Generate a preliminary qt_lib_XXX_private.pri file
    file(GENERATE
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${private_pri_file_name}"
        CONTENT
        "QT.${config_module_name}_private.VERSION = ${PROJECT_VERSION}
QT.${config_module_name}_private.name = ${module}
QT.${config_module_name}_private.module =
QT.${config_module_name}_private.libs = $$QT_MODULE_LIB_BASE
QT.${config_module_name}_private.includes = ${private_module_includes}
QT.${config_module_name}_private.frameworks = ${private_module_frameworks}
QT.${config_module_name}_private.depends = ${private_module_dependencies}
QT.${config_module_name}_private.uses =
QT.${config_module_name}_private.module_config = ${joined_module_internal_config}
QT.${config_module_name}_private.enabled_features = ${enabled_private_features}
QT.${config_module_name}_private.disabled_features = ${disabled_private_features}"
    )

    if(QT_GENERATOR_IS_MULTI_CONFIG)
        set(configs ${CMAKE_CONFIGURATION_TYPES})
    else()
        set(configs ${CMAKE_BUILD_TYPE})
    endif()
    set(inputs "${CMAKE_CURRENT_BINARY_DIR}/${private_pri_file_name}")
    foreach(cfg ${configs})
        list(APPEND inputs "${CMAKE_CURRENT_BINARY_DIR}/${cfg}/${pri_data_cmake_file}")
    endforeach()

    qt_path_join(private_pri_file_path "${target_path}" "${private_pri_file_name}")
    list(APPEND pri_files "${private_pri_file_path}")
    add_custom_command(
        OUTPUT "${private_pri_file_path}"
        DEPENDS ${inputs}
        COMMAND ${CMAKE_COMMAND} "-DIN_FILES=${inputs}" "-DOUT_FILE=${private_pri_file_path}"
                "-DCONFIGS=${configs}"
                -P "${QT_CMAKE_DIR}/QtGenerateLibPri.cmake"
        VERBATIM)
    add_custom_target(${target}_lib_pri DEPENDS "${private_pri_file_path}")
    if(arg_HEADER_MODULE)
        add_dependencies(${target}_timestamp ${target}_lib_pri)
    else()
        add_dependencies(${target} ${target}_lib_pri)
    endif()
    qt_install(FILES "${pri_files}" DESTINATION ${INSTALL_MKSPECSDIR}/modules)
endfunction()

# Generates qt_ext_XXX.pri files for consumption by qmake
function(qt_generate_3rdparty_lib_pri_file target lib pri_file_var)
    if(NOT lib)
        # Don't write a pri file for projects that don't set QMAKE_LIB_NAME yet.
        return()
    endif()

    if(QT_GENERATOR_IS_MULTI_CONFIG)
        set(configs ${CMAKE_CONFIGURATION_TYPES})
    else()
        set(configs ${CMAKE_BUILD_TYPE})
    endif()

    file(GENERATE
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/qt_ext_${lib}.cmake"
        CONTENT "set(cfg $<CONFIG>)
set(incdir $<TARGET_PROPERTY:${target},INTERFACE_INCLUDE_DIRECTORIES>)
set(defines $<TARGET_PROPERTY:${target},INTERFACE_COMPILE_DEFINITIONS>)
set(libs $<TARGET_FILE:${target}>)
")

    set(inputs "")
    foreach(cfg ${configs})
        list(APPEND inputs "${CMAKE_CURRENT_BINARY_DIR}/${cfg}/qt_ext_${lib}.cmake")
    endforeach()

    qt_path_join(pri_target_path ${QT_BUILD_DIR} ${INSTALL_MKSPECSDIR}/modules)
    qt_path_join(pri_file "${pri_target_path}" "qt_ext_${lib}.pri")
    qt_path_join(qt_build_libdir ${QT_BUILD_DIR} ${INSTALL_LIBDIR})
    add_custom_command(
        OUTPUT "${pri_file}"
        DEPENDS ${inputs}
        COMMAND ${CMAKE_COMMAND} "-DIN_FILES=${inputs}" "-DOUT_FILE=${pri_file}" -DLIB=${lib}
                "-DCONFIGS=${configs}"
                "-DQT_BUILD_LIBDIR=${qt_build_libdir}"
                -P "${QT_CMAKE_DIR}/QtGenerateExtPri.cmake"
        VERBATIM)
    add_custom_target(${target}_ext_pri DEPENDS "${pri_file}")
    add_dependencies(${target} ${target}_ext_pri)
    set(${pri_file_var} ${pri_file} PARENT_SCOPE)
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

# Generates qt_plugin_XXX.pri files for consumption by qmake
#
# QT_PLUGIN.XXX.EXTENDS is set to "-" for the following plugin types:
#   - generic
#   - platform, if the plugin is not the default QPA plugin
# Otherwise, this variable is empty.
function(qt_generate_plugin_pri_file target pri_file_var)
    get_target_property(plugin_name ${target} OUTPUT_NAME)
    get_target_property(plugin_type ${target} QT_PLUGIN_TYPE)
    get_target_property(default_plugin ${target} QT_DEFAULT_PLUGIN)
    get_target_property(plugin_class_name ${target} QT_PLUGIN_CLASS_NAME)

    set(plugin_extends "")
    if(NOT default_plugin AND (plugin_type STREQUAL "generic" OR plugin_type STREQUAL "platforms"))
        set(plugin_extends "-")
    endif()

    set(plugin_deps "")
    get_target_property(target_deps ${target} _qt_target_deps)
    foreach(dep ${target_deps})
        list(GET dep 0 dep_name)
        qt_get_qmake_module_name(dep_name ${dep_name})
        list(APPEND plugin_deps ${dep_name})
    endforeach()
    list(REMOVE_DUPLICATES plugin_deps)
    list(JOIN plugin_deps " " plugin_deps)

    qt_path_join(pri_target_path ${QT_BUILD_DIR} ${INSTALL_MKSPECSDIR}/modules)
    qt_path_join(pri_file "${pri_target_path}" "qt_plugin_${plugin_name}.pri")
    qt_configure_file(OUTPUT "${pri_file}" CONTENT "QT_PLUGIN.${plugin_name}.TYPE = ${plugin_type}
QT_PLUGIN.${plugin_name}.EXTENDS = ${plugin_extends}
QT_PLUGIN.${plugin_name}.DEPENDS = ${plugin_deps}
QT_PLUGIN.${plugin_name}.CLASS_NAME = ${plugin_class_name}
QT_PLUGINS += ${plugin_name}
")
    set(${pri_file_var} "${pri_file}" PARENT_SCOPE)
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

function(qt_correct_features out_var features)
    set(corrected_features "")
    foreach(feature ${features})
        get_property(feature_original_name GLOBAL PROPERTY "QT_FEATURE_ORIGINAL_NAME_${feature}")
        list(APPEND corrected_features "${feature_original_name}")
    endforeach()
    set(${out_var} ${corrected_features} PARENT_SCOPE)
endfunction()

# Get original names for config values (which correspond to feature names) and use them if they
# exist, otherwise just use the config value (which might be the case when a config value has
# a custom name).
function(qt_correct_config out_var config)
    set(corrected_config "")
    foreach(name ${config})
        # Is the config value a known feature?
        get_property(feature_original_name GLOBAL PROPERTY "QT_FEATURE_ORIGINAL_NAME_${name}")
        if(feature_original_name)
            list(APPEND corrected_config "${feature_original_name}")
            continue()
        endif()

        # Is the config value a negated known feature, e.g. no_foo?
        # Then add the config value no-foo.
        if(name MATCHES "^no_(.*)")
            get_property(feature_original_name GLOBAL PROPERTY
                "QT_FEATURE_ORIGINAL_NAME_${CMAKE_MATCH_1}")
            if(feature_original_name)
                list(APPEND corrected_config "no-${feature_original_name}")
                continue()
            endif()
        endif()

        # The config value is no known feature. Add the value as is.
        list(APPEND corrected_config "${name}")
    endforeach()
    set(${out_var} ${corrected_config} PARENT_SCOPE)
endfunction()

# Creates mkspecs/qconfig.pri which contains public global features among other things.
function(qt_generate_global_config_pri_file)
    qt_path_join(qconfig_pri_target_path ${PROJECT_BINARY_DIR} ${INSTALL_MKSPECSDIR})
    qt_path_join(qconfig_pri_target_path "${qconfig_pri_target_path}" "qconfig.pri")

    get_target_property(enabled_features GlobalConfig INTERFACE_QT_ENABLED_PUBLIC_FEATURES)
    get_target_property(disabled_features GlobalConfig INTERFACE_QT_DISABLED_PUBLIC_FEATURES)

    qt_correct_features(corrected_enabled_features "${enabled_features}")
    qt_correct_features(corrected_disabled_features "${disabled_features}")

    string (REPLACE ";" " " corrected_enabled_features "${corrected_enabled_features}")
    string (REPLACE ";" " " corrected_disabled_features "${corrected_disabled_features}")

    # Add some required CONFIG entries.
    set(config_entries "")
    if(CMAKE_BUILD_TYPE STREQUAL Debug)
        list(APPEND config_entries "debug")
    elseif(CMAKE_BUILD_TYPE STREQUAL Release)
        list(APPEND config_entries "release")
    endif()
    list(APPEND config_entries "${qt_build_config_type}")
    string (REPLACE ";" " " config_entries "${config_entries}")

    get_target_property(public_config GlobalConfig INTERFACE_QT_QMAKE_PUBLIC_CONFIG)
    get_target_property(qt_public_config GlobalConfig INTERFACE_QT_QMAKE_PUBLIC_QT_CONFIG)
    qt_correct_config(corrected_public_config "${public_config}")
    qt_correct_config(corrected_qt_public_config "${qt_public_config}")
    qt_guess_qmake_build_config(qmake_build_config)
    list(APPEND corrected_qt_public_config ${qmake_build_config})

    list(JOIN corrected_public_config " " public_config_joined)
    list(JOIN corrected_qt_public_config " " qt_public_config_joined)

    file(GENERATE
        OUTPUT "${qconfig_pri_target_path}"
        CONTENT
        "QT_ARCH = ${TEST_architecture_arch}
QT_BUILDABI = ${TEST_buildAbi}
QT.global.enabled_features = ${corrected_enabled_features}
QT.global.disabled_features = ${corrected_disabled_features}
QT.global.disabled_features += release build_all
QT_CONFIG += ${qt_public_config_joined}
CONFIG += ${config_entries} ${public_config_joined}
QT_VERSION = ${PROJECT_VERSION}
QT_MAJOR_VERSION = ${PROJECT_VERSION_MAJOR}
QT_MINOR_VERSION = ${PROJECT_VERSION_MINOR}
QT_PATCH_VERSION = ${PROJECT_VERSION_PATCH}
"
    )
    qt_install(FILES "${qconfig_pri_target_path}" DESTINATION ${INSTALL_MKSPECSDIR})
endfunction()

function(qt_get_build_parts out_var)
    set(parts "libs")

    if(BUILD_EXAMPLES AND NOT QT_NO_MAKE_EXAMPLES)
        list(APPEND parts "examples")
    endif()

    if(BUILD_TESTING AND NOT QT_NO_MAKE_TESTS)
        list(APPEND parts "tests")
    endif()

    if(NOT CMAKE_CROSSCOMPILING OR QT_BUILD_TOOLS_WHEN_CROSSCOMPILING)
        list(APPEND parts "tools")
    endif()

    set(${out_var} ${parts} PARENT_SCOPE)
endfunction()

# Creates mkspecs/qmodule.pri which contains private global features among other things.
function(qt_generate_global_module_pri_file)
    qt_path_join(qmodule_pri_target_path ${PROJECT_BINARY_DIR} ${INSTALL_MKSPECSDIR})
    qt_path_join(qmodule_pri_target_path "${qmodule_pri_target_path}" "qmodule.pri")

    get_target_property(enabled_features GlobalConfig INTERFACE_QT_ENABLED_PRIVATE_FEATURES)
    get_target_property(disabled_features GlobalConfig INTERFACE_QT_DISABLED_PRIVATE_FEATURES)

    qt_correct_features(corrected_enabled_features "${enabled_features}")
    qt_correct_features(corrected_disabled_features "${disabled_features}")

    string (REPLACE ";" " " corrected_enabled_features "${corrected_enabled_features}")
    string (REPLACE ";" " " corrected_disabled_features "${corrected_disabled_features}")

    set(corrected_private_config "")
    get_target_property(private_config GlobalConfig INTERFACE_QT_QMAKE_PRIVATE_CONFIG)
    qt_correct_config(corrected_private_config "${private_config}")
    list(JOIN corrected_private_config " " private_config_joined)

    set(content "QT.global_private.enabled_features = ${corrected_enabled_features}
QT.global_private.disabled_features = ${corrected_disabled_features}
CONFIG += ${private_config_joined}
")
    if(PKG_CONFIG_FOUND)
        string(APPEND content "PKG_CONFIG_EXECUTABLE = ${PKG_CONFIG_EXECUTABLE}\n")
    endif()

    # TODO: Write QT_COORD_TYPE once we support setting it.

    qt_get_build_parts(build_parts)
    string(REPLACE ";" " " build_parts "${build_parts}")
    string(APPEND content "QT_BUILD_PARTS = ${build_parts}\n")

    set(preliminary_pri_root "${CMAKE_CURRENT_BINARY_DIR}/mkspecs/preliminary")
    set(pri_data_cmake_file "qmodule.cmake")
    qt_generate_qmake_libraries_pri_content(global ${preliminary_pri_root} ${pri_data_cmake_file})

    # Generate a preliminary qmodule.pri file
    set(preliminary_pri_file_path "${preliminary_pri_root}/qmodule.pri")
    file(GENERATE
        OUTPUT ${preliminary_pri_file_path}
        CONTENT "${content}"
    )

    if(QT_GENERATOR_IS_MULTI_CONFIG)
        set(configs ${CMAKE_CONFIGURATION_TYPES})
    else()
        set(configs ${CMAKE_BUILD_TYPE})
    endif()
    set(inputs ${preliminary_pri_file_path})
    foreach(cfg ${configs})
        list(APPEND inputs "${preliminary_pri_root}/${cfg}/${pri_data_cmake_file}")
    endforeach()

    add_custom_command(
        OUTPUT "${qmodule_pri_target_path}"
        DEPENDS ${inputs}
        COMMAND ${CMAKE_COMMAND} "-DIN_FILES=${inputs}" "-DOUT_FILE=${qmodule_pri_target_path}"
                "-DCONFIGS=${configs}"
                -P "${QT_CMAKE_DIR}/QtGenerateLibPri.cmake"
        VERBATIM)
    add_custom_target(qmodule_pri DEPENDS "${qmodule_pri_target_path}")
    qt_install(FILES "${qmodule_pri_target_path}" DESTINATION ${INSTALL_MKSPECSDIR})
endfunction()

function(qt_generate_qt_conf)
    qt_path_join(qt_conf__path ${PROJECT_BINARY_DIR} "bin" "qt.conf")

    file(GENERATE
        OUTPUT "${qt_conf__path}"
        CONTENT
        "[EffectivePaths]
Prefix=..
[DevicePaths]
Prefix=${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}
[Paths]
Prefix=${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}
HostPrefix=${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}
Sysroot=
SysrootifyPrefix=false
TargetSpec=${QT_QMAKE_TARGET_MKSPEC}
HostSpec=${QT_QMAKE_HOST_MKSPEC}
[EffectiveSourcePaths]
Prefix=${CMAKE_SOURCE_DIR}
")
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
        set_target_properties("${target}Versionless" PROPERTIES
            EXPORT_NAME "${target}"
            _qt_is_versionless_target "TRUE")
        set_property(TARGET "${target}Versionless"
                     APPEND PROPERTY EXPORT_PROPERTIES _qt_is_versionless_target)

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
    # Also create a namespaced alias of the form ${target}::${target}_nolink which is used by
    # our modules.
    # Also create a Qt namespaced alias target, because when exporting via install(EXPORT)
    # Vulkan::Vulkan_nolink transforms into Qt6::Vulkan_nolink, and the latter needs to be an
    # accessible alias for standalone tests.
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
        add_library("${INSTALL_CMAKE_NAMESPACE}::${nolink_target}" ALIAS ${nolink_target})

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

        qt_path_join(syncqt_install_dir ${QT_INSTALL_DIR} ${INSTALL_BINDIR})
        qt_copy_or_install(PROGRAMS "${SYNCQT_FROM_SOURCE}"
                           DESTINATION "${syncqt_install_dir}")
    elseif(QT_HOST_PATH)
        get_filename_component(syncqt_absolute_path
                               "${QT_HOST_PATH}/${INSTALL_LIBEXECDIR}/syncqt.pl"
                               ABSOLUTE)
        set(QT_SYNCQT "${syncqt_absolute_path}" CACHE FILEPATH "syncqt script")
        message(STATUS "Using host syncqt found at: ${QT_SYNCQT}")
    else()
        get_filename_component(syncqt_absolute_path
                               "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${INSTALL_LIBEXECDIR}/syncqt.pl"
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

include(CheckCXXSourceCompiles)

function(qt_internal_add_link_flags_no_undefined target)
    if (NOT QT_BUILD_SHARED_LIBS)
        return()
    endif()
    if (GCC OR CLANG)
        set(previous_CMAKE_REQUIRED_LINK_OPTIONS ${CMAKE_REQUIRED_LINK_OPTIONS})

        set(CMAKE_REQUIRED_LINK_OPTIONS "-Wl,-undefined,error")
        check_cxx_source_compiles("int main() {}" HAVE_DASH_UNDEFINED_ERROR)
        if(HAVE_DASH_UNDEFINED_ERROR)
            set(no_undefined_flag "-Wl,-undefined,error")
        endif()

        set(CMAKE_REQUIRED_LINK_OPTIONS "-Wl,--no-undefined")
        check_cxx_source_compiles("int main() {}" HAVE_DASH_DASH_NO_UNDEFINED)
        if(HAVE_DASH_DASH_NO_UNDEFINED)
            set(no_undefined_flag "-Wl,--no-undefined")
        endif()

        set(CMAKE_REQUIRED_LINK_OPTIONS ${previous_CMAKE_REQUIRED_LINK_OPTIONS})

        if (NOT HAVE_DASH_UNDEFINED_ERROR AND NOT HAVE_DASH_DASH_NO_UNDEFINED)
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
                COMMAND "${HOST_PERL}" "${QT_MKSPECS_DIR}/features/data/unix/findclasslist.pl" < "${infile}" > "${outfile}"
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
    set("${result}_repo_include_dir" "${QT_BUILD_DIR}/${INSTALL_INCLUDEDIR}" PARENT_SCOPE)
    set("${result}_include_dir" "${QT_BUILD_DIR}/${INSTALL_INCLUDEDIR}/${module}" PARENT_SCOPE)
    set("${result}_define" "${define}" PARENT_SCOPE)
endfunction()


set(__default_private_args "SOURCES;LIBRARIES;INCLUDE_DIRECTORIES;DEFINES;DBUS_ADAPTOR_BASENAME;DBUS_ADAPTOR_FLAGS;DBUS_ADAPTOR_SOURCES;DBUS_INTERFACE_BASENAME;DBUS_INTERFACE_FLAGS;DBUS_INTERFACE_SOURCES;FEATURE_DEPENDENCIES;COMPILE_OPTIONS;LINK_OPTIONS;MOC_OPTIONS;DISABLE_AUTOGEN_TOOLS;ENABLE_AUTOGEN_TOOLS;PLUGIN_TYPES")

set(__default_public_args "PUBLIC_LIBRARIES;PUBLIC_INCLUDE_DIRECTORIES;PUBLIC_DEFINES;PUBLIC_COMPILE_OPTIONS;PUBLIC_LINK_OPTIONS")
set(__default_private_module_args "PRIVATE_MODULE_INTERFACE")
set(__default_target_info_args TARGET_VERSION TARGET_PRODUCT TARGET_DESCRIPTION TARGET_COMPANY
    TARGET_COPYRIGHT)

option(QT_CMAKE_DEBUG_EXTEND_TARGET "Debug extend_target calls in Qt's build system" OFF)

# Initial autogen setup for a target to specify certain CMake properties which are common
# to all autogen tools. Also enable AUTOMOC by default.
function(qt_autogen_tools_initial_setup target)
    set_property(TARGET "${target}" PROPERTY INTERFACE_QT_MAJOR_VERSION ${PROJECT_VERSION_MAJOR})
    set_property(TARGET "${target}" APPEND PROPERTY COMPATIBLE_INTERFACE_STRING QT_MAJOR_VERSION)

    set_directory_properties(PROPERTIES
        QT_VERSION_MAJOR ${PROJECT_VERSION_MAJOR}
        QT_VERSION_MINOR ${PROJECT_VERSION_MINOR}
        QT_VERSION_PATCH ${PROJECT_VERSION_PATCH}
    )

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

    # Only process private dependencies if target is a static library
    get_target_property(target_type ${target} TYPE)
    set(lib_list ${public_libs})
    if (target_type STREQUAL "STATIC_LIBRARY")
        list(APPEND lib_list ${private_libs})
    endif()

    foreach(lib IN LISTS lib_list)
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
        set_property(TARGET "${target}" APPEND PROPERTY "PRECOMPILE_HEADERS" "$<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:OBJCXX>>:${precompiled_header}>")
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
    # No obj-cxx PCH support for versions lower than 3.16.
    if(CMAKE_VERSION VERSION_LESS 3.16.0)
        list(FILTER sources INCLUDE REGEX "\\.mm$")
        qt_update_ignore_pch_source("${target}" "${sources}")
    endif()
endfunction()

# This function can be used to add sources/libraries/etc. to the specified CMake target
# if the provided CONDITION evaluates to true.
function(qt_extend_target target)
    # Don't try to extend_target when cross compiling an imported host target (like a tool).
    qt_is_imported_target("${target}" is_imported)
    if(is_imported)
        return()
    endif()

    if (NOT TARGET "${target}")
        message(FATAL_ERROR "Trying to extend non-existing target \"${target}\".")
    endif()
    qt_parse_all_arguments(arg "qt_extend_target" "HEADER_MODULE" "PRECOMPILED_HEADER"
        "CONDITION;${__default_public_args};${__default_private_args};${__default_private_module_args};COMPILE_FLAGS;NO_PCH_SOURCES" ${ARGN})
    if ("x${arg_CONDITION}" STREQUAL x)
        set(arg_CONDITION ON)
    endif()

    qt_evaluate_config_expression(result ${arg_CONDITION})
    if (${result})
        if(QT_CMAKE_DEBUG_EXTEND_TARGET)
            message("qt_extend_target(${target} CONDITION ${arg_CONDITION} ...): Evaluated")
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
            set_property (TARGET "${target}" APPEND PROPERTY
                AUTOMOC_MOC_OPTIONS "${arg_MOC_OPTIONS}"
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
          target_link_libraries("${target_private}"
                                INTERFACE ${arg_PRIVATE_MODULE_INTERFACE})
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
            message("qt_extend_target(${target} CONDITION ${arg_CONDITION} ...): Skipped")
        endif()
    endif()
endfunction()


function(qt_internal_library_deprecation_level result)
    if(WIN32)
        # On Windows, due to the way DLLs work, we need to export all functions,
        # including the inlines
        list(APPEND deprecations "QT_DISABLE_DEPRECATED_BEFORE=0x040800")
    else()
        # On other platforms, Qt's own compilation goes needs to compile the Qt 5.0 API
        list(APPEND deprecations "QT_DISABLE_DEPRECATED_BEFORE=0x050000")
    endif()
    list(APPEND deprecations "QT_DEPRECATED_WARNINGS_SINCE=0x060000")
    set("${result}" deprecations PARENT_SCOPE)
endfunction()


function(qt_install_injections target build_dir install_dir)
    set(injections ${ARGN})
    set(module "Qt${target}")
    get_target_property(target_type ${target} TYPE)
    if (target_type STREQUAL "INTERFACE_LIBRARY")
        set(is_framework FALSE)
    else()
        get_target_property(is_framework ${target} FRAMEWORK)
    endif()
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

        qt_configure_file(OUTPUT "${lower_case_forwarding_header_path}/${original_file_name}"
             CONTENT "${main_contents}")

        if(is_framework)
            if(file MATCHES "_p\\.h$")
                set(header_type PRIVATE)
            else()
                set(header_type PUBLIC)
            endif()
            qt_copy_framework_headers(${target} ${header_type}
                ${current_repo_build_dir}/${file})
        else()
            # Copy the actual injected (generated) header file (not the just created forwarding one)
            # to its install location when doing a prefix build. In an non-prefix build, the qt_install
            # will be a no-op.
            qt_path_join(install_destination
                        ${install_dir} ${INSTALL_INCLUDEDIR} ${module} ${destinationdir})
            qt_install(FILES ${current_repo_build_dir}/${file}
                    DESTINATION ${install_destination}
                    RENAME ${destinationname} OPTIONAL)
        endif()

        # Generate UpperCaseNamed forwarding headers (part 3).
        foreach(fwd_hdr ${fwd_hdrs})
            set(upper_case_forwarding_header_path "${INSTALL_INCLUDEDIR}/${module}")
            if(destinationdir)
                string(APPEND upper_case_forwarding_header_path "/${destinationdir}")
            endif()

            # Generate upper case forwarding header like QVulkanFunctions or QtConfig.
            qt_configure_file(OUTPUT "${build_dir}/${upper_case_forwarding_header_path}/${fwd_hdr}"
                 CONTENT "#include \"${destinationname}\"\n")

            if(is_framework)
                # Copy the forwarding header to the framework's Headers directory.
                qt_copy_framework_headers(${target} PUBLIC
                    "${build_dir}/${upper_case_forwarding_header_path}/${fwd_hdr}")
            else()
                # Install the forwarding header.
                qt_path_join(install_destination
                            ${install_dir} ${upper_case_forwarding_header_path})
                qt_install(FILES "${build_dir}/${upper_case_forwarding_header_path}/${fwd_hdr}"
                        DESTINATION ${install_destination} OPTIONAL)
            endif()
        endforeach()
    endforeach()
endfunction()


function(qt_read_headers_pri module_include_dir resultVarPrefix)
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

function(qt_skip_warnings_are_errors target)
    get_target_property(target_type "${target}" TYPE)
    if(target_type STREQUAL "INTERFACE_LIBRARY")
        return()
    endif()
    set_target_properties("${target}" PROPERTIES QT_SKIP_WARNINGS_ARE_ERRORS ON)
endfunction()

function(qt_skip_warnings_are_errors_when_repo_unclean target)
    if(QT_REPO_NOT_WARNINGS_CLEAN)
        qt_skip_warnings_are_errors("${target}")
    endif()
endfunction()

function(qt_disable_warnings target)
    get_target_property(target_type "${target}" TYPE)
    if(target_type STREQUAL "INTERFACE_LIBRARY")
        return()
    endif()
    set_target_properties("${target}" PROPERTIES QT_COMPILE_OPTIONS_DISABLE_WARNINGS ON)
endfunction()

function(qt_set_symbol_visibility_preset target value)
    get_target_property(target_type "${target}" TYPE)
    if(target_type STREQUAL "INTERFACE_LIBRARY")
        return()
    endif()

    set_target_properties("${target}" PROPERTIES C_VISIBILITY_PRESET "${value}")
    set_target_properties("${target}" PROPERTIES CXX_VISIBILITY_PRESET "${value}")
endfunction()

function(qt_set_symbol_visibility_hidden target)
    qt_set_symbol_visibility_preset("${target}" "hidden")
endfunction()

function(qt_get_sanitized_plugin_type plugin_type out_var)
    # Used to handle some edge cases such as platforms/darwin
    string(REGEX REPLACE "[-/]" "_" plugin_type "${plugin_type}")
    set("${out_var}" "${plugin_type}" PARENT_SCOPE)
endfunction()

# Copy header files to QtXYZ.framework/Versions/6/Headers/
# Use this function for header files that
#   - are not added as source files to the target
#   - are not marked as PUBLIC_HEADER
#   - or are private and supposed to end up in the 6.7.8/QtXYZ/private subdir.
function(qt_copy_framework_headers target)
    get_target_property(is_fw ${target} FRAMEWORK)
    if(NOT "${is_fw}")
        return()
    endif()

    set(options PUBLIC PRIVATE QPA)
    set(oneValueArgs)
    set(multiValueArgs)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    get_target_property(fw_version ${target} FRAMEWORK_VERSION)
    get_target_property(fw_bundle_version ${target} MACOSX_FRAMEWORK_BUNDLE_VERSION)
    get_target_property(fw_dir ${target} LIBRARY_OUTPUT_DIRECTORY)
    get_target_property(fw_name ${target} OUTPUT_NAME)
    set(fw_headers_dir ${fw_dir}/${fw_name}.framework/Versions/${fw_version}/Headers/)
    if(ARG_PRIVATE)
        string(APPEND fw_headers_dir "${fw_bundle_version}/Qt${target}/private/")
    elseif(ARG_QPA)
        string(APPEND fw_headers_dir "${fw_bundle_version}/Qt${target}/qpa/")
    endif()

    set(out_files)
    foreach(hdr IN LISTS ARG_UNPARSED_ARGUMENTS)
        get_filename_component(in_file_path ${hdr} ABSOLUTE)
        get_filename_component(in_file_name ${hdr} NAME)
        set(out_file_path ${fw_headers_dir}${in_file_name})
        add_custom_command(
            OUTPUT ${out_file_path}
            DEPENDS ${in_file_path}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${fw_headers_dir}"
            COMMAND ${CMAKE_COMMAND} -E copy "${in_file_path}" "${fw_headers_dir}")
        list(APPEND out_files ${out_file_path})
    endforeach()

    get_target_property(fw_copied_headers ${target} QT_COPIED_FRAMEWORK_HEADERS)
    if(NOT fw_copied_headers)
        set(fw_copied_headers "")
    endif()
    list(APPEND fw_copied_headers ${out_files})
    set_target_properties(${target} PROPERTIES QT_COPIED_FRAMEWORK_HEADERS "${fw_copied_headers}")
endfunction()

function(qt_finalize_framework_headers_copy target)
    get_target_property(target_type ${target} TYPE)
    if(${target_type} STREQUAL "INTERFACE_LIBRARY")
        return()
    endif()
    get_target_property(is_fw ${target} FRAMEWORK)
    if(NOT "${is_fw}")
        return()
    endif()
    get_target_property(headers ${target} QT_COPIED_FRAMEWORK_HEADERS)
    if(headers)
        # Hack to create the "Headers" symlink in the framework:
        # Create a fake header file and copy it into the framework by marking it as PUBLIC_HEADER.
        # CMake now takes care of creating the symlink.
        set(fake_header ${target}_fake_header.h)
        qt_get_main_cmake_configuration(main_config)
        file(GENERATE OUTPUT ${fake_header} CONTENT "// ignore this file\n"
             CONDITION "$<CONFIG:${main_config}>")
        string(PREPEND fake_header "${CMAKE_CURRENT_BINARY_DIR}/")
        target_sources(${target} PRIVATE ${fake_header})
        set_source_files_properties(${fake_header} PROPERTIES GENERATED ON)
        set_property(TARGET ${target} APPEND PROPERTY PUBLIC_HEADER ${fake_header})

        # Add a target, e.g. Core_framework_headers, that triggers the header copy.
        add_custom_target(${target}_framework_headers DEPENDS ${headers})
        add_dependencies(${target} ${target}_framework_headers)
    endif()
endfunction()

function(qt_get_cmake_configurations out_var)
    set(possible_configs "${CMAKE_BUILD_TYPE}")
    if(CMAKE_CONFIGURATION_TYPES)
        set(possible_configs "${CMAKE_CONFIGURATION_TYPES}")
    endif()
    set(${out_var} "${possible_configs}" PARENT_SCOPE)
endfunction()

function(qt_clone_property_for_configs target property configs)
    get_target_property(value "${target}" "${property}")
    foreach(config ${configs})
        string(TOUPPER "${config}" upper_config)
        set_property(TARGET "${target}" PROPERTY "${property}_${upper_config}" "${value}")
    endforeach()
endfunction()

function(qt_handle_multi_config_output_dirs target)
    qt_get_cmake_configurations(possible_configs)
    qt_clone_property_for_configs(${target} LIBRARY_OUTPUT_DIRECTORY "${possible_configs}")
    qt_clone_property_for_configs(${target} RUNTIME_OUTPUT_DIRECTORY "${possible_configs}")
    qt_clone_property_for_configs(${target} ARCHIVE_OUTPUT_DIRECTORY "${possible_configs}")
endfunction()

# Add a finalizer function for the current CMake list file.
#
# You may add up to nine arguments that are passed to the finalizer.
# A finalizer that is registered with qt_add_list_file_finalizer(foo bar baz)
# will be called with nine arguments: foo(bar baz IGNORE IGNORE IGNORE...),
# because CMake's handling of empty list elements is a cruel joke.
#
# For CMake < 3.18 the function qt_watch_current_list_dir must know about the finalizer.
function(qt_add_list_file_finalizer func)
    set_property(GLOBAL APPEND
        PROPERTY QT_LIST_FILE_FINALIZER_FILES "${CMAKE_CURRENT_LIST_FILE}")
    set_property(GLOBAL APPEND
        PROPERTY QT_LIST_FILE_FINALIZER_FUNCS ${func})
    foreach(i RANGE 1 9)
        set(arg "${ARGV${i}}")
        if(i GREATER_EQUAL ARGC OR "${arg}" STREQUAL "")
            set(arg "IGNORE")
        endif()
        set_property(GLOBAL APPEND
            PROPERTY QT_LIST_FILE_FINALIZER_ARGV${i} "${arg}")
    endforeach()
endfunction()

# Watcher function for the variable CMAKE_CURRENT_LIST_DIR.
# This is the driver of the finalizer facility.
function(qt_watch_current_list_dir variable access value current_list_file stack)
    if(NOT access STREQUAL "MODIFIED_ACCESS")
        # We are only interested in modifications of CMAKE_CURRENT_LIST_DIR.
        return()
    endif()
    list(GET stack -1 stack_top)
    if(stack_top STREQUAL current_list_file)
        # If the top of the stack equals the current list file then
        # we're entering a file. We're not interested in this case.
        return()
    endif()
    get_property(files GLOBAL PROPERTY QT_LIST_FILE_FINALIZER_FILES)
    if(NOT files)
        return()
    endif()
    get_property(funcs GLOBAL PROPERTY QT_LIST_FILE_FINALIZER_FUNCS)
    foreach(i RANGE 1 9)
        get_property(args${i} GLOBAL PROPERTY QT_LIST_FILE_FINALIZER_ARGV${i})
    endforeach()
    list(LENGTH files n)
    set(i 0)
    while(i LESS n)
        list(GET files ${i} file)
        if(file STREQUAL stack_top)
            list(GET funcs ${i} func)
            foreach(k RANGE 1 9)
                list(GET args${k} ${i} a${k})
            endforeach()
            # We've found a file we're looking for. Call the finalizer.
            if(${CMAKE_VERSION} VERSION_LESS "3.18.0")
                # Make finalizer known functions here:
                if(func STREQUAL "qt_finalize_module")
                    qt_finalize_module(${a1} ${a2} ${a3} ${a4} ${a5} ${a6} ${a7} ${a8} ${a9})
                else()
                    message(FATAL_ERROR "qt_watch_current_list_dir doesn't know about ${func}. Consider adding it.")
                endif()
            else()
                cmake_language(CALL ${func} ${a1} ${a2} ${a3} ${a4} ${a5} ${a6} ${a7} ${a8} ${a9})
            endif()
            list(REMOVE_AT files ${i})
            list(REMOVE_AT funcs ${i})
            foreach(k RANGE 1 9)
                list(REMOVE_AT args${k} ${i})
            endforeach()
            math(EXPR n "${n} - 1")
        else()
            math(EXPR i "${i} + 1")
        endif()
    endwhile()
    set_property(GLOBAL PROPERTY QT_LIST_FILE_FINALIZER_FILES ${files})
    set_property(GLOBAL PROPERTY QT_LIST_FILE_FINALIZER_FUNCS ${funcs})
    foreach(i RANGE 1 9)
        set_property(GLOBAL PROPERTY QT_LIST_FILE_FINALIZER_ARGV${i} "${args${i}}")
    endforeach()
endfunction()

variable_watch(CMAKE_CURRENT_LIST_DIR qt_watch_current_list_dir)

# Set target properties that are the same for all modules, plugins, executables
# and 3rdparty libraries.
function(qt_set_common_target_properties target)
   if(FEATURE_static_runtime)
       if(MSVC)
           set_property(TARGET ${target} PROPERTY
               MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
       elseif(MINGW)
           target_link_options(${target} INTERFACE "LINKER:-static")
       endif()
   endif()
endfunction()

# Set common, informational target properties.
#
# On Windows, these properties are used to generate the version information resource.
function(qt_set_target_info_properties target)
    cmake_parse_arguments(arg "" "${__default_target_info_args}" "" ${ARGN})
    if("${arg_TARGET_VERSION}" STREQUAL "")
        set(arg_TARGET_VERSION "${PROJECT_VERSION}.0")
    endif()
    if("${arg_TARGET_PRODUCT}" STREQUAL "")
        set(arg_TARGET_PRODUCT "Qt6")
    endif()
    if("${arg_TARGET_DESCRIPTION}" STREQUAL "")
        set(arg_TARGET_DESCRIPTION "C++ Application Development Framework")
    endif()
    if("${arg_TARGET_COMPANY}" STREQUAL "")
        set(arg_TARGET_COMPANY "The Qt Company Ltd.")
    endif()
    if("${arg_TARGET_COPYRIGHT}" STREQUAL "")
        set(arg_TARGET_COPYRIGHT "Copyright (C) 2020 The Qt Company Ltd.")
    endif()
    set_target_properties(${target} PROPERTIES
        QT_TARGET_VERSION "${arg_TARGET_VERSION}"
        QT_TARGET_COMPANY_NAME "${arg_TARGET_COMPANY}"
        QT_TARGET_DESCRIPTION "${arg_TARGET_DESCRIPTION}"
        QT_TARGET_COPYRIGHT "${arg_TARGET_COPYRIGHT}"
        QT_TARGET_PRODUCT_NAME "${arg_TARGET_PRODUCT}")
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
function(qt_add_module target)
    qt_internal_module_info(module "${target}")

    # Process arguments:
    qt_parse_all_arguments(arg "qt_add_module"
        "NO_MODULE_HEADERS;STATIC;DISABLE_TOOLS_EXPORT;EXCEPTIONS;INTERNAL_MODULE;NO_SYNC_QT;NO_PRIVATE_MODULE;HEADER_MODULE;GENERATE_METATYPES;NO_CONFIG_HEADER_FILE;SKIP_DEPENDS_INCLUDE"
        "MODULE_INCLUDE_NAME;CONFIG_MODULE_NAME;PRECOMPILED_HEADER;${__default_target_info_args}"
        "${__default_private_args};${__default_public_args};${__default_private_module_args};QMAKE_MODULE_CONFIG;EXTRA_CMAKE_FILES;EXTRA_CMAKE_INCLUDES;NO_PCH_SOURCES" ${ARGN})

    qt_internal_add_qt_repo_known_module("${target}")

    if(NOT DEFINED arg_CONFIG_MODULE_NAME)
        set(arg_CONFIG_MODULE_NAME "${module_lower}")
    endif()

    ### Define Targets:
    set(is_interface_lib 0)
    set(is_shared_lib 0)
    if(${arg_HEADER_MODULE})
        add_library("${target}" INTERFACE)
        set(is_interface_lib 1)
    elseif(${arg_STATIC})
        add_library("${target}" STATIC)
    elseif(${QT_BUILD_SHARED_LIBS})
        add_library("${target}" SHARED)
        set(is_shared_lib 1)
    else()
        add_library("${target}" STATIC)
    endif()

    set(property_prefix "INTERFACE_")
    if(NOT arg_HEADER_MODULE)
        qt_set_common_target_properties(${target})
        set(property_prefix "")
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
            FRAMEWORK_VERSION ${PROJECT_VERSION_MAJOR}
            MACOSX_FRAMEWORK_IDENTIFIER org.qt-project.Qt${target}
            MACOSX_FRAMEWORK_BUNDLE_VERSION ${PROJECT_VERSION}
            MACOSX_FRAMEWORK_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        )
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

    if(QT_FEATURE_separate_debug_info AND is_shared_lib AND (UNIX OR MINGW))
        qt_enable_separate_debug_info(${target} ${INSTALL_LIBDIR})
    endif()

    if (ANDROID)
        qt_android_apply_arch_suffix("${target}")
    endif()
    qt_internal_add_target_aliases("${target}")
    qt_skip_warnings_are_errors_when_repo_unclean("${target}")

    # Add _private target to link against the private headers:
    if(NOT ${arg_NO_PRIVATE_MODULE})
        set(target_private "${target}Private")
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

        if (arg_SKIP_DEPENDS_INCLUDE)
            set_target_properties(${target} PROPERTIES QT_MODULE_SKIP_DEPENDS_INCLUDE TRUE)
        endif()
        if(is_framework)
            set_target_properties(${target} PROPERTIES
                OUTPUT_NAME Qt${target}
            )
        else()
            set_target_properties(${target} PROPERTIES
                OUTPUT_NAME "${INSTALL_CMAKE_NAMESPACE}${target}"
            )
        endif()

        if (WIN32 AND BUILD_SHARED_LIBS)
            qt6_generate_win32_rc_file(${target})
        endif()
    endif()

    # Module headers:
    if(${arg_NO_MODULE_HEADERS} OR ${arg_NO_SYNC_QT})
        set_target_properties("${target}" PROPERTIES INTERFACE_MODULE_HAS_HEADERS OFF)
    else()
        if(arg_MODULE_INCLUDE_NAME)
            set(module_include_name ${arg_MODULE_INCLUDE_NAME})
        else()
            set(module_include_name ${module})
        endif()
        set_target_properties("${target}" PROPERTIES INTERFACE_MODULE_INCLUDE_NAME "${module_include_name}")

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
        execute_process(COMMAND ${syncqt_full_command})

        set_target_properties("${target}" PROPERTIES INTERFACE_MODULE_HAS_HEADERS ON)

        ### FIXME: Can we replace headers.pri?
        set(module_include_dir "${QT_BUILD_DIR}/${INSTALL_INCLUDEDIR}/${module_include_name}")
        qt_read_headers_pri("${module_include_dir}" "module_headers")
        set(module_depends_header "${module_include_dir}/${module}Depends")
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
            set_property(TARGET "${target}" PROPERTY MODULE_HEADER "${module_include_dir}/${module_include_name}")
        endif()

        if(module_headers_qpa)
            if(is_framework)
                qt_copy_framework_headers(${target} QPA "${module_headers_qpa}")
            else()
                qt_install(
                    FILES ${module_headers_qpa}
                    DESTINATION ${INSTALL_INCLUDEDIR}/${module}/${PROJECT_VERSION}/${module_include_name}/qpa)
            endif()
        endif()
    endif()

    if(NOT arg_HEADER_MODULE)
        # This property is used for super builds with static libraries. We use
        # it in QtPlugins.cmake.in to avoid "polluting" the dependency chain
        # for the target in it's project directory.
        # E.g: When we process find_package(Qt6 ... Gui) in QtDeclarative, the
        # rules in QtPugins.cmake add all the known Gui plugins as interface
        # dependencies. This in turn causes circular dependencies on every
        # plugin which links against Gui. Plugin A -> GUI -> Plugin A ....

        set_target_properties(${target} PROPERTIES QT_BUILD_PROJECT_NAME ${PROJECT_NAME})
        # Plugin types associated to a module
        if(NOT "x${arg_PLUGIN_TYPES}" STREQUAL "x")
            # Reset the variable containing the list of plugins for the given plugin type
            foreach(plugin_type ${arg_PLUGIN_TYPES})
                qt_get_sanitized_plugin_type("${plugin_type}" plugin_type)
                set_property(TARGET "${target}" APPEND PROPERTY MODULE_PLUGIN_TYPES "${plugin_type}")
                qt_internal_add_qt_repo_known_plugin_types("${plugin_type}")
            endforeach()
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
        set(fw_bundle_subdir "${INSTALL_LIBDIR}/Qt${target}.framework")
        set(fw_headers_subdir "Versions/${PROJECT_VERSION_MAJOR}/Headers")
        list(APPEND public_includes
            # Add the lib/Foo.framework dir as include path to let CMake generate
            # the -F compiler flag.
            "$<BUILD_INTERFACE:${QT_BUILD_DIR}/${fw_bundle_subdir}>"
            "$<INSTALL_INTERFACE:${fw_bundle_subdir}>"

            # Add the fully resolved Headers subdir, because the Headers symlink might
            # not be there yet.
            "$<BUILD_INTERFACE:${QT_BUILD_DIR}/${fw_bundle_subdir}/${fw_headers_subdir}>"

            # After installing, the Headers symlink is guaranteed to exist.
            "$<INSTALL_INTERFACE:${fw_bundle_subdir}/Headers>"
            )
    endif()

    # Handle cases like QmlDevTools which do not have their own headers, but rather borrow them
    # from another module.
    if(NOT arg_NO_SYNC_QT AND NOT arg_NO_MODULE_HEADERS)
        # Don't include private headers unless they exist, aka syncqt created them.
        if(module_headers_private)
            list(APPEND private_includes
                        "$<BUILD_INTERFACE:${module_include_dir}/${PROJECT_VERSION}>"
                        "$<BUILD_INTERFACE:${module_include_dir}/${PROJECT_VERSION}/${module}>")
        endif()

        list(APPEND ${public_headers_list}
                    # For the syncqt headers
                    "$<BUILD_INTERFACE:${module_repo_include_dir}>"
                    "$<BUILD_INTERFACE:${module_include_dir}>")
    endif()

    if(NOT arg_NO_MODULE_HEADERS AND NOT arg_NO_SYNC_QT)
        # For the syncqt headers
        list(APPEND ${public_headers_list} "$<INSTALL_INTERFACE:${INSTALL_INCLUDEDIR}/${module}>")
    endif()
    list(APPEND ${public_headers_list} ${arg_PUBLIC_INCLUDE_DIRECTORIES})

    set(header_module)
    if(arg_HEADER_MODULE)
        set(header_module "HEADER_MODULE")

        # Provide a *_timestamp target that can be used to trigger the build of custom_commands.
        set(timestamp_file "${CMAKE_CURRENT_BINARY_DIR}/timestamp")
        add_custom_command(OUTPUT "${timestamp_file}"
            COMMAND ${CMAKE_COMMAND} -E touch "${timestamp_file}"
            DEPENDS ${module_headers_public}
            VERBATIM)
        add_custom_target(${target}_timestamp ALL DEPENDS "${timestamp_file}")
    endif()

    qt_extend_target("${target}"
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

    if(NOT ${arg_EXCEPTIONS} AND NOT ${arg_HEADER_MODULE})
        qt_internal_set_no_exceptions_flags("${target}")
    endif()

    set(configureFile "${CMAKE_CURRENT_SOURCE_DIR}/configure.cmake")
    if(EXISTS "${configureFile}" AND NOT arg_NO_CONFIG_HEADER_FILE)
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

    # Generate metatypes
    set(QT_MODULE_HAS_META_TYPES_FILE FALSE)
    if (${arg_GENERATE_METATYPES})
        set(QT_MODULE_HAS_META_TYPES_FILE TRUE)
        set(metatypes_install_dir ${INSTALL_LIBDIR}/metatypes)
        set(args)
        if (NOT QT_WILL_INSTALL)
            set(args COPY_OVER_INSTALL INSTALL_DIR "${QT_BUILD_DIR}/${metatypes_install_dir}")
        else()
            set(args INSTALL_DIR "${metatypes_install_dir}")
        endif()
        qt6_generate_meta_types_json_file(${target} ${args})
    endif()
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
        FRAMEWORK DESTINATION ${INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${INSTALL_INCLUDEDIR}/${module}
        PRIVATE_HEADER DESTINATION ${INSTALL_INCLUDEDIR}/${module}/${PROJECT_VERSION}/${module}/private
        )

    qt_apply_rpaths(TARGET "${target}" INSTALL_PATH "${INSTALL_LIBDIR}" RELATIVE_RPATH)

    if (ANDROID AND NOT arg_HEADER_MODULE)
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

    ### fixme: cmake is missing a built-in variable for this. We want to apply it only to modules and plugins
    # that belong to Qt.
    if(NOT arg_HEADER_MODULE)
        qt_internal_add_link_flags_no_undefined("${target}")
    endif()

    set(interface_includes "")

    # Handle cases like QmlDevTools which do not have their own headers, but rather borrow them
    # from another module.
    if(NOT arg_NO_SYNC_QT)
        list(APPEND interface_includes "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>")

        # syncqt.pl does not create a private header directory like 'include/6.0/QtFoo' unless
        # the module has foo_p.h header files. For QtZlib, there are no such private headers, so we
        # need to make sure not to add such include paths unless the directory exists, otherwise
        # consumers of the module will fail at CMake generation time stating that
        # INTERFACE_INCLUDE_DIRECTORIES contains a non-existent path.
        if(NOT arg_NO_MODULE_HEADERS
                AND EXISTS "${module_include_dir}/${PROJECT_VERSION}/${module}")
            list(APPEND interface_includes
                        "$<BUILD_INTERFACE:${module_include_dir}/${PROJECT_VERSION}>"
                        "$<BUILD_INTERFACE:${module_include_dir}/${PROJECT_VERSION}/${module}>")

            if(is_framework)
                set(fw_headers_dir
                    "${INSTALL_LIBDIR}/${module}.framework/Headers/")
                list(APPEND interface_includes
                            "$<INSTALL_INTERFACE:${fw_headers_dir}${PROJECT_VERSION}>"
                            "$<INSTALL_INTERFACE:${fw_headers_dir}${PROJECT_VERSION}/${module}>")
            else()
                list(APPEND interface_includes
                            "$<INSTALL_INTERFACE:${INSTALL_INCLUDEDIR}/${module}/${PROJECT_VERSION}>"
                            "$<INSTALL_INTERFACE:${INSTALL_INCLUDEDIR}/${module}/${PROJECT_VERSION}/${module}>")
            endif()
        endif()
    endif()

    if(NOT ${arg_NO_PRIVATE_MODULE})
        target_include_directories("${target_private}" INTERFACE ${interface_includes})
        target_link_libraries("${target_private}" INTERFACE "${target}")
    endif()

    if(is_framework AND NOT is_interface_lib)
        qt_finalize_framework_headers_copy(${target})
    endif()

    qt_describe_module(${target})
    qt_add_list_file_finalizer(qt_finalize_module ${target} ${arg_INTERNAL_MODULE} ${header_module})
endfunction()

function(qt_finalize_module target)
    qt_generate_prl_file(${target})
    qt_generate_module_pri_file("${target}" ${ARGN})
endfunction()

# Add libraries to variable ${out_libs_var} in a way that duplicates
# are added at the end. This ensures the library order needed for the
# linker.
function(qt_merge_libs out_libs_var)
    foreach(dep ${ARGN})
        list(REMOVE_ITEM ${out_libs_var} ${dep})
        list(APPEND ${out_libs_var} ${dep})
    endforeach()
    set(${out_libs_var} ${${out_libs_var}} PARENT_SCOPE)
endfunction()

# Collects the library dependencies of a target.
# This takes into account transitive usage requirements.
function(qt_collect_libs target out_var)
    set(collected ${ARGN})
    if(target IN_LIST collected)
        return()
    endif()
    list(APPEND collected ${target})
    if(NOT TARGET qt_collect_libs_dict)
        add_library(qt_collect_libs_dict INTERFACE IMPORTED GLOBAL)
    endif()
    get_target_property(libs qt_collect_libs_dict INTERFACE_${target})
    if(NOT libs)
        unset(libs)
        get_target_property(target_libs ${target} INTERFACE_LINK_LIBRARIES)
        if(NOT target_libs)
            unset(target_libs)
        endif()
        get_target_property(target_type ${target} TYPE)
        if(target_type STREQUAL "STATIC_LIBRARY")
            get_target_property(link_libs ${target} LINK_LIBRARIES)
            if(link_libs)
                list(APPEND target_libs ${link_libs})
            endif()
        endif()
        foreach(lib ${target_libs})
            # Cannot use $<TARGET_POLICY:...> in add_custom_command.
            # Check the policy now, and replace the generator expression with the value.
            while(lib MATCHES "\\$<TARGET_POLICY:([^>]+)>")
                cmake_policy(GET ${CMAKE_MATCH_1} value)
                if(value STREQUAL "NEW")
                    set(value "TRUE")
                else()
                    set(value "FALSE")
                endif()
                string(REPLACE "${CMAKE_MATCH_0}" "${value}" lib "${lib}")
            endwhile()

            # Fix up $<TARGET_PROPERTY:FOO> expressions that refer to the "current" target.
            # Those cannot be used with add_custom_command.
            while(lib MATCHES "\\$<TARGET_PROPERTY:([^,>]+)>")
                string(REPLACE "${CMAKE_MATCH_0}" "$<TARGET_PROPERTY:${target},${CMAKE_MATCH_1}>"
                    lib "${lib}")
            endwhile()

            if(lib MATCHES "^\\$<TARGET_OBJECTS:")
                # Skip object files.
                continue()
            elseif(lib MATCHES "^\\$<LINK_ONLY:(.*)>$")
                set(lib_target ${CMAKE_MATCH_1})
            else()
                set(lib_target ${lib})
            endif()

            if(TARGET ${lib_target})
                if ("${lib_target}" MATCHES "^Qt::(.*)")
                    # If both, Qt::Foo and Foo targets exist, prefer the target name without
                    # namespace. Which one is preferred doesn't really matter. This code exists to
                    # avoid ending up with both, Qt::Foo and Foo in our dependencies.
                    set(namespaceless_lib_target "${CMAKE_MATCH_1}")
                    if(TARGET namespaceless_lib_target)
                        set(lib_target ${namespaceless_lib_target})
                    endif()
                endif()
                get_target_property(lib_target_type ${lib_target} TYPE)
                if(lib_target_type STREQUAL "INTERFACE_LIBRARY")
                    qt_collect_libs(${lib_target} lib_libs ${collected})
                    if(lib_libs)
                        qt_merge_libs(libs ${lib_libs})
                        set(is_module 0)
                    endif()
                else()
                    qt_merge_libs(libs "$<TARGET_FILE:${lib_target}>")
                    qt_collect_libs(${lib_target} lib_libs ${collected})
                    if(lib_libs)
                        qt_merge_libs(libs ${lib_libs})
                    endif()
                endif()
            else()
                set(final_lib_name_to_merge "${lib_target}")
                if(lib_target MATCHES "/([^/]+).framework$")
                    set(final_lib_name_to_merge "-framework ${CMAKE_MATCH_1}")
                endif()
                qt_merge_libs(libs "${final_lib_name_to_merge}")
            endif()
        endforeach()
        set_target_properties(qt_collect_libs_dict PROPERTIES INTERFACE_${target} "${libs}")
    endif()
    set(${out_var} ${libs} PARENT_SCOPE)
endfunction()

# Generate a qmake .prl file for the given target
function(qt_generate_prl_file target)
    get_target_property(target_type ${target} TYPE)
    if(target_type STREQUAL "INTERFACE_LIBRARY")
        return()
    endif()

    unset(prl_libs)
    qt_collect_libs(${target} prl_libs)

    unset(prl_config)
    if(target_type STREQUAL "STATIC_LIBRARY")
        list(APPEND prl_config static)
    elseif(target_type STREQUAL "SHARED_LIBRARY")
        list(APPEND prl_config shared)
    endif()
    if (NOT target_type STREQUAL "INTERFACE_LIBRARY")
        get_target_property(is_fw ${target} FRAMEWORK)
        if(is_fw)
            list(APPEND prl_config lib_bundle)
        endif()
    endif()
    list(JOIN prl_config " " prl_config)

    # Generate a preliminary .prl file that contains absolute paths to all libraries
    set(prl_file_name "$<TARGET_FILE_PREFIX:${target}>$<TARGET_FILE_BASE_NAME:${target}>.prl")
    file(GENERATE
        OUTPUT "${prl_file_name}"
        CONTENT
        "QMAKE_PRL_BUILD_DIR = ${CMAKE_CURRENT_BINARY_DIR}
QMAKE_PRL_TARGET = $<TARGET_FILE_NAME:${target}>
QMAKE_PRL_CONFIG = ${prl_config}
QMAKE_PRL_VERSION = ${PROJECT_VERSION}
QMAKE_PRL_LIBS_FOR_CMAKE = ${prl_libs}
"
    )

    # Add a custom command that prepares the .prl file for installation
    qt_path_join(qt_build_libdir ${QT_BUILD_DIR} ${INSTALL_LIBDIR})
    qt_path_join(prl_file_path "${qt_build_libdir}" "${prl_file_name}")
    set(library_suffixes ${CMAKE_SHARED_LIBRARY_SUFFIX} ${CMAKE_STATIC_LIBRARY_SUFFIX})
    add_custom_command(
        TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -DIN_FILE=${prl_file_name} -DOUT_FILE=${prl_file_path}
                "-DLIBRARY_SUFFIXES=${library_suffixes}"
                -DQT_BUILD_LIBDIR=${qt_build_libdir}
                -P "${QT_CMAKE_DIR}/QtFinishPrlFile.cmake"
        VERBATIM
        )

    # Installation of the .prl file happens globally elsewhere,
    # because we have no clue here what the actual file name is.
endfunction()

function(qt_export_tools module_name)
    # Bail out when cross-compiling, unless QT_BUILD_TOOLS_WHEN_CROSSCOMPILING is on.
    if(CMAKE_CROSSCOMPILING AND NOT QT_BUILD_TOOLS_WHEN_CROSSCOMPILING)
        return()
    endif()

    # If no tools were defined belonging to this module, don't create a config and targets file.
    if(NOT "${module_name}" IN_LIST QT_KNOWN_MODULES_WITH_TOOLS)
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
    set(tool_targets_non_prefixed "")

    # List of package dependencies that need be find_package'd when using the Tools package.
    set(package_deps "")

    foreach(tool_name ${QT_KNOWN_MODULE_${module_name}_TOOLS})
        # Specific tools can have package dependencies.
        # e.g. qtwaylandscanner depends on WaylandScanner (non-qt package).
        get_target_property(extra_packages "${tool_name}" QT_EXTRA_PACKAGE_DEPENDENCIES)
        if(extra_packages)
            list(APPEND package_deps "${extra_packages}")
        endif()

        if (CMAKE_CROSSCOMPILING AND QT_BUILD_TOOLS_WHEN_CROSSCOMPILING)
            string(REGEX REPLACE "_native$" "" tool_name ${tool_name})
        endif()
        set(extra_cmake_statements "${extra_cmake_statements}
if (NOT QT_NO_CREATE_TARGETS)
    get_property(is_global TARGET ${INSTALL_CMAKE_NAMESPACE}::${tool_name} PROPERTY IMPORTED_GLOBAL)
    if(NOT is_global)
        set_property(TARGET ${INSTALL_CMAKE_NAMESPACE}::${tool_name} PROPERTY IMPORTED_GLOBAL TRUE)
    endif()
endif()
")
        list(APPEND tool_targets "${QT_CMAKE_EXPORT_NAMESPACE}::${tool_name}")
        list(APPEND tool_targets_non_prefixed "${tool_name}")
    endforeach()

    string(APPEND extra_cmake_statements
"set(${QT_CMAKE_EXPORT_NAMESPACE}${module_name}Tools_TARGETS \"${tool_targets}\")")

    # Extract package dependencies that were determined in QtPostProcess, but only if ${module_name}
    # is an actual target.
    # module_name can be a non-existent target, if the tool doesn't have an existing associated
    # module, e.g. qtwaylandscanner.
    if(TARGET "${module_name}")
        get_target_property(module_package_deps "${module_name}" _qt_tools_package_deps)
        if(module_package_deps)
            list(APPEND package_deps "${module_package_deps}")
        endif()
    endif()

    # Configure and install dependencies file for the ${module_name}Tools package.
    configure_file(
        "${QT_CMAKE_DIR}/QtModuleToolsDependencies.cmake.in"
        "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}Dependencies.cmake"
        @ONLY
    )

    qt_install(FILES
        "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}Dependencies.cmake"
        DESTINATION "${config_install_dir}"
        COMPONENT Devel
    )

    # Configure and install the ${module_name}Tools package Config file.
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


    # Create versionless targets file.
    configure_file(
        "${QT_CMAKE_DIR}/QtModuleToolsVersionlessTargets.cmake.in"
        "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}VersionlessTargets.cmake"
        @ONLY
    )

    qt_install(FILES
        "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}VersionlessTargets.cmake"
        DESTINATION "${config_install_dir}"
        COMPONENT Devel
    )
endfunction()

# This function records a dependency between ${target_name} and ${dep_package_name}.
# at the CMake package level.
# E.g. The Tools package that provides the qtwaylandscanner target
# needs to call find_package(WaylandScanner) (non-qt-package).
# main_target_name = qtwaylandscanner
# dep_package_name = WaylandScanner
function(qt_record_extra_package_dependency main_target_name dep_package_name dep_package_version)
    if (TARGET "${main_target_name}")
        get_target_property(extra_packages "${main_target_name}" QT_EXTRA_PACKAGE_DEPENDENCIES)
        if(NOT extra_packages)
            set(extra_packages "")
        endif()

        list(APPEND extra_packages "${dep_package_name}\;${dep_package_version}")
        set_target_properties("${main_target_name}" PROPERTIES QT_EXTRA_PACKAGE_DEPENDENCIES
                                                               "${extra_packages}")
    endif()
endfunction()

# This function records a dependency between ${main_target_name} and ${dep_target_name}
# at the CMake package level.
# E.g. Qt6CoreConfig.cmake needs to find_package(Qt6WinMain).
# main_target_name = Core
# dep_target_name = WinMain
# This is just a convenience function that deals with Qt targets and their associated packages
# instead of raw package names.
function(qt_record_extra_qt_package_dependency main_target_name dep_target_name
                                                                dep_package_version)
    # WinMain -> Qt6WinMain.
    qt_internal_module_info(qtfied_target_name "${dep_target_name}")
    qt_record_extra_package_dependency("${main_target_name}" "${qtfied_target_name_versioned}"
                                                             "${dep_package_version}")
endfunction()

function(qt_internal_check_directory_or_type name dir type default result_var)
    if ("x${dir}" STREQUAL x)
        if("x${type}" STREQUAL x)
            message(FATAL_ERROR "qt_internal_add_plugin called without setting either TYPE or ${name}.")
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

    qt_get_sanitized_plugin_type("${target_type}" target_type)
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


# Collection of qt_add_plugin arguments so they can be shared across different
# plugin type wrappers
set(__qt_add_plugin_optional_args
    "STATIC;EXCEPTIONS;ALLOW_UNDEFINED_SYMBOLS"
)
set(__qt_add_plugin_single_args
    "TYPE;CLASS_NAME;OUTPUT_DIRECTORY;INSTALL_DIRECTORY;ARCHIVE_INSTALL_DIRECTORY;QML_TARGET_PATH;OUTPUT_NAME"
    ${__default_target_info_args}
)
set(__qt_add_plugin_multi_args
    "${__default_private_args};${__default_public_args};DEFAULT_IF"
)
# This is the main entry point for defining Qt plugins.
# A CMake target is created with the given target. The TYPE parameter is needed to place the
# plugin into the correct plugins/ sub-directory.
function(qt_internal_add_plugin target)
    qt_internal_module_info(module "${target}")

    qt_internal_set_qt_known_plugins("${QT_KNOWN_PLUGINS}" "${target}")

    qt_parse_all_arguments(arg "qt_internal_add_plugin"
        "${__qt_add_plugin_optional_args};SKIP_INSTALL"
        "${__qt_add_plugin_single_args}"
        "${__qt_add_plugin_multi_args}"
        "${ARGN}"
    )

    qt_get_sanitized_plugin_type("${arg_TYPE}" plugin_type_escaped)

    set(output_directory_default "${QT_BUILD_DIR}/${INSTALL_PLUGINSDIR}/${arg_TYPE}")
    set(install_directory_default "${INSTALL_PLUGINSDIR}/${arg_TYPE}")
    set(archive_install_directory_default "${INSTALL_LIBDIR}/${arg_TYPE}")

    if (arg_QML_TARGET_PATH)
        set(target_path "${arg_QML_TARGET_PATH}")
        set(output_directory_default "${QT_BUILD_DIR}/${INSTALL_QMLDIR}/${target_path}")
        set(install_directory_default "${INSTALL_QMLDIR}/${target_path}")
        set(archive_install_directory_default "${INSTALL_QMLDIR}/${target_path}")
    endif()

    # Derive the class name from the target name if it's not explicitly specified.
    # Don't set it for qml plugins though.
    set(plugin_class_name "")
    if (NOT "${plugin_type_escaped}" STREQUAL "qml_plugin")
        if (NOT arg_CLASS_NAME)
            set(plugin_class_name "${target}")
        else()
            set(plugin_class_name "${arg_CLASS_NAME}")
        endif()
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
        if(WIN32)
            # CMake sets for Windows-GNU platforms the suffix "lib"
            set_property(TARGET "${target}" PROPERTY PREFIX "")
        endif()
    endif()

    qt_set_common_target_properties(${target})
    qt_set_target_info_properties(${target} ${ARGN} TARGET_VERSION "${arg_VERSION}")

    # Make sure the Qt6 plugin library names are like they were in Qt5 qmake land.
    # Whereas the Qt6 CMake target names are like the Qt5 CMake target names.
    set(output_name "${target}")
    if(arg_OUTPUT_NAME)
        set(output_name "${arg_OUTPUT_NAME}")
    endif()
    set_property(TARGET "${target}" PROPERTY OUTPUT_NAME "${output_name}")

    # Add a custom target with the Qt5 qmake name for a more user friendly ninja experience.
    if(arg_OUTPUT_NAME AND NOT TARGET "${output_name}")
        add_custom_target("${output_name}")
        add_dependencies("${output_name}" "${target}")
    endif()

    if (ANDROID)
        qt_android_apply_arch_suffix("${target}")
        set_target_properties(${target}
            PROPERTIES
            LIBRARY_OUTPUT_NAME "plugins_${arg_TYPE}_${output_name}"
        )
    endif()
    qt_internal_add_target_aliases("${target}")
    qt_skip_warnings_are_errors_when_repo_unclean("${target}")

    # Disable linking of plugins against other plugins during static regular and
    # super builds. The latter causes cyclic dependencies otherwise.
    set_target_properties(${target} PROPERTIES QT_DEFAULT_PLUGINS 0)

    set_target_properties("${target}" PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${output_directory}"
        RUNTIME_OUTPUT_DIRECTORY "${output_directory}"
        ARCHIVE_OUTPUT_DIRECTORY "${output_directory}"
        QT_PLUGIN_TYPE "${plugin_type_escaped}"
        QT_PLUGIN_CLASS_NAME "${plugin_class_name}")
        qt_handle_multi_config_output_dirs("${target}")

    qt_internal_library_deprecation_level(deprecation_define)

    qt_autogen_tools_initial_setup(${target})

    set(static_plugin_define "")
    if (arg_STATIC OR NOT QT_BUILD_SHARED_LIBS)
        set(static_plugin_define "QT_STATICPLUGIN")
    endif()

    # Save the Qt module in the plug-in's properties
    if(NOT plugin_type_escaped STREQUAL "qml_plugin")
        qt_get_module_for_plugin("${target}" "${plugin_type_escaped}")
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

    qt_extend_target("${target}"
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
    qt_generate_plugin_pri_file("${target}" pri_file)

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
        qt_install(FILES
            "${pri_file}"
            DESTINATION "${INSTALL_MKSPECSDIR}/modules"
        )

        # Make the export name of plugins be consistent with modules, so that
        # qt_add_resource adds its additional targets to the same export set in a static Qt build.
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
        qt_apply_rpaths(TARGET "${target}" INSTALL_PATH "${install_directory}" RELATIVE_RPATH)
    endif()

    if (NOT arg_ALLOW_UNDEFINED_SYMBOLS)
        ### fixme: cmake is missing a built-in variable for this. We want to apply it only to
        # modules and plugins that belong to Qt.
        qt_internal_add_link_flags_no_undefined("${target}")
    endif()

    qt_internal_add_linker_version_script(${target})
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


function(qt_add_resource target resourceName)
    # Don't try to add resources when cross compiling, and the target is actually a host target
    # (like a tool).
    qt_is_imported_target("${target}" is_imported)
    if(is_imported)
        return()
    endif()

    qt_parse_all_arguments(arg "qt_add_resource" "" "PREFIX;LANG;BASE" "FILES" ${ARGN})

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

# Collection of qt_add_executable arguments so they can be shared across qt_add_executable
# and qt_add_test_helper.
set(__qt_add_executable_optional_args
    "GUI;BOOTSTRAP;NO_QT;NO_INSTALL;EXCEPTIONS"
)
set(__qt_add_executable_single_args
    "OUTPUT_DIRECTORY;INSTALL_DIRECTORY;VERSION"
    ${__default_target_info_args}
)
set(__qt_add_executable_multi_args
    "EXE_FLAGS;${__default_private_args};${__default_public_args}"
)

# This function creates a CMake target for a generic console or GUI binary.
# Please consider to use a more specific version target like the one created
# by qt_add_test or qt_add_tool below.
function(qt_add_executable name)
    qt_parse_all_arguments(arg "qt_add_executable"
        "${__qt_add_executable_optional_args}"
        "${__qt_add_executable_single_args}"
        "${__qt_add_executable_multi_args}"
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
        # On our qmake builds we don't compile the executables with
        # visibility=hidden. Not having this flag set will cause the
        # executable to have main() hidden and can then no longer be loaded
        # through dlopen()
        set_property(TARGET ${name} PROPERTY C_VISIBILITY_PRESET default)
        set_property(TARGET ${name} PROPERTY CXX_VISIBILITY_PRESET default)
    else()
        add_executable("${name}" ${arg_EXE_FLAGS})
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
    qt_set_target_info_properties(${name} ${ARGN} TARGET_VERSION "${arg_VERSION}")

    if (WIN32)
        qt6_generate_win32_rc_file(${name})
    endif()

    qt_set_common_target_properties(${name})
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

    qt_extend_target("${name}"
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
    if(NOT ${arg_EXCEPTIONS})
        qt_internal_set_no_exceptions_flags("${name}")
    endif()

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
            qt_install(TARGETS "${name}"
                       ${additional_install_args} # Needs to be before the DESTINATIONS.
                       CONFIGURATIONS ${cmake_config}
                       ${install_targets_default_args})
        endforeach()
    endif()
endfunction()

# Simple wrapper around qt_add_executable for benchmarks which insure that
# the binary is built under ${CMAKE_CURRENT_BINARY_DIR} and never installed.
# See qt_add_executable() for more details.
function(qt_add_benchmark target)

    qt_parse_all_arguments(arg "qt_add_benchmark"
        "${__qt_add_executable_optional_args}"
        "${__qt_add_executable_single_args}"
        "${__qt_add_executable_multi_args}"
        ${ARGN}
    )

    qt_remove_args(exec_args
        ARGS_TO_REMOVE
            ${target}
            OUTPUT_DIRECTORY
            INSTALL_DIRECTORY
        ALL_ARGS
            "${__qt_add_executable_optional_args}"
            "${__qt_add_executable_single_args}"
            "${__qt_add_executable_multi_args}"
        ARGS
            ${ARGV}
    )

    if(NOT arg_OUTPUT_DIRECTORY)
        set(arg_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    endif()

    qt_add_executable(${target}
        NO_INSTALL # we don't install benchmarks
        OUTPUT_DIRECTORY "${arg_OUTPUT_DIRECTORY}" # avoid polluting bin directory
        ${exec_args}
    )

endfunction()

# Simple wrapper around qt_add_executable for manual tests which insure that
# the binary is built under ${CMAKE_CURRENT_BINARY_DIR} and never installed.
# See qt_add_executable() for more details.
function(qt_add_manual_test target)

    qt_parse_all_arguments(arg "qt_add_manual_test"
        "${__qt_add_executable_optional_args}"
        "${__qt_add_executable_single_args}"
        "${__qt_add_executable_multi_args}"
        ${ARGN}
    )

    qt_remove_args(exec_args
        ARGS_TO_REMOVE
            ${target}
            OUTPUT_DIRECTORY
            INSTALL_DIRECTORY
        ALL_ARGS
            "${__qt_add_executable_optional_args}"
            "${__qt_add_executable_single_args}"
            "${__qt_add_executable_multi_args}"
        ARGS
            ${ARGV}
    )

    if(NOT arg_OUTPUT_DIRECTORY)
        set(arg_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    endif()

    qt_add_executable(${target}
        NO_INSTALL # we don't install benchmarks
        OUTPUT_DIRECTORY "${arg_OUTPUT_DIRECTORY}" # avoid polluting bin directory
        ${exec_args}
    )

endfunction()


# This function creates a CMake test target with the specified name for use with CTest.
function(qt_add_test name)
    qt_parse_all_arguments(arg "qt_add_test"
        "RUN_SERIAL;EXCEPTIONS;GUI;QMLTEST"
        "OUTPUT_DIRECTORY;WORKING_DIRECTORY;TIMEOUT;VERSION"
        "QML_IMPORTPATH;TESTDATA;${__default_private_args};${__default_public_args}" ${ARGN}
    )

    if (NOT arg_OUTPUT_DIRECTORY)
        set(arg_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    endif()

    if (${arg_EXCEPTIONS})
        set(exceptions_text "EXCEPTIONS")
    endif()

    if (${arg_GUI})
        set(gui_text "GUI")
    endif()

    if (arg_VERSION)
        set(version_arg VERSION "${arg_VERSION}")
    endif()

    # Handle cases where we have a qml test without source files
    if (arg_SOURCES)
        set(private_includes
            "${CMAKE_CURRENT_SOURCE_DIR}"
            "${CMAKE_CURRENT_BINARY_DIR}"
            "$<BUILD_INTERFACE:${QT_BUILD_DIR}/${INSTALL_INCLUDEDIR}>"
             ${arg_INCLUDE_DIRECTORIES}
        )

        qt_add_executable("${name}"
            ${exceptions_text}
            ${gui_text}
            ${version_arg}
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

        qt_extend_target("${name}" CONDITION arg_QMLTEST
            PUBLIC_LIBRARIES ${QT_CMAKE_EXPORT_NAMESPACE}::QuickTest
        )

        qt_extend_target("${name}" CONDITION arg_QMLTEST AND NOT ANDROID
            DEFINES
                QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
        )

        qt_extend_target("${name}" CONDITION arg_QMLTEST AND ANDROID
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

    if (ANDROID)
        qt_android_add_test("${name}")
    else()
        if(arg_QMLTEST AND NOT arg_SOURCES)
            set(test_working_dir "${CMAKE_CURRENT_SOURCE_DIR}")
            set(test_executable ${QT_CMAKE_EXPORT_NAMESPACE}::qmltestrunner)
        else()
            if (arg_WORKING_DIRECTORY)
                set(test_working_dir "${arg_WORKING_DIRECTORY}")
            elseif(arg_OUTPUT_DIRECTORY)
                set(test_working_dir "${arg_OUTPUT_DIRECTORY}")
            else()
                set(test_working_dir "${CMAKE_CURRENT_BINARY_DIR}")
            endif()
            set(test_executable "${name}")
        endif()

        add_test(NAME "${name}" COMMAND ${test_executable} ${extra_test_args} -o ${name}.xml,xml -o -,txt  WORKING_DIRECTORY "${test_working_dir}")
    endif()
    set_tests_properties("${name}" PROPERTIES RUN_SERIAL "${arg_RUN_SERIAL}" LABELS "${label}")
    if (arg_TIMEOUT)
        set_tests_properties(${name} PROPERTIES TIMEOUT ${arg_TIMEOUT})
    endif()

    # Add a ${target}/check makefile target, to more easily test one test.
    if(TEST "${name}")
        add_custom_target("${name}_check"
            VERBATIM
            COMMENT "Running ${CMAKE_CTEST_COMMAND} -V -R \"^${name}$\""
            COMMAND "${CMAKE_CTEST_COMMAND}" -V -R "^${name}$"
            )
        if(TARGET "${name}")
            add_dependencies("${name}_check" "${name}")
        endif()
    endif()

    # Get path to <qt_relocatable_install_prefix>/bin, as well as CMAKE_INSTALL_PREFIX/bin, then
    # prepend them to the PATH environment variable.
    # It's needed on Windows to find the shared libraries and plugins.
    # qt_relocatable_install_prefix is dynamically computed from the location of where the Qt CMake
    # package is found.
    # The regular CMAKE_INSTALL_PREFIX can be different for example when building standalone tests.
    # Any given CMAKE_INSTALL_PREFIX takes priority over qt_relocatable_install_prefix for the
    # PATH environment variable.
    set(install_prefixes "${CMAKE_INSTALL_PREFIX}")
    if(QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX)
        list(APPEND install_prefixes "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}")
    endif()

    set(test_env_path "PATH=${CMAKE_CURRENT_BINARY_DIR}")
    foreach(install_prefix ${install_prefixes})
        set(test_env_path "${test_env_path}${QT_PATH_SEPARATOR}${install_prefix}/${INSTALL_BINDIR}")
    endforeach()
    set(test_env_path "${test_env_path}${QT_PATH_SEPARATOR}$ENV{PATH}")
    string(REPLACE ";" "\;" test_env_path "${test_env_path}")
    set_property(TEST "${name}" APPEND PROPERTY ENVIRONMENT "${test_env_path}")
    set_property(TEST "${name}" APPEND PROPERTY ENVIRONMENT "QT_TEST_RUNNING_IN_CTEST=1")

    # Add the install prefix to list of plugin paths when doing a prefix build
    if(NOT QT_INSTALL_DIR)
        foreach(install_prefix ${install_prefixes})
            list(APPEND plugin_paths "${install_prefix}/${INSTALL_PLUGINSDIR}")
        endforeach()
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
                qt_add_resource(${name} "${name}_testdata_builtin"
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
                qt_install(
                    DIRECTORY "${testdata}"
                    DESTINATION "${testdata_install_dir}")
            else()
                qt_install(
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
function(qt_add_test_helper name)

    set(qt_add_test_helper_optional_args
        "OVERRIDE_OUTPUT_DIRECTORY"
    )

    qt_parse_all_arguments(arg "qt_add_test_helper"
        "${qt_add_test_helper_optional_args};${__qt_add_executable_optional_args}"
        "${__qt_add_executable_single_args}"
        "${__qt_add_executable_multi_args}"
         ${ARGN})

    qt_remove_args(forward_args
        ARGS_TO_REMOVE
            "${name}"
            ${qt_add_test_helper_optional_args}
        ALL_ARGS
            ${qt_add_test_helper_optional_args}
            ${__qt_add_executable_optional_args}
            ${__qt_add_executable_single_args}
            ${__qt_add_executable_multi_args}
        ARGS
            ${ARGV}
    )

    set(extra_args_to_pass)
    if(NOT arg_OVERRIDE_OUTPUT_DIRECTORY)
        set(extra_args_to_pass OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/..")
    endif()

    qt_add_executable("${name}" NO_INSTALL ${extra_args_to_pass} ${forward_args})
endfunction()

# Sets QT_WILL_BUILD_TOOLS if tools will be built.
function(qt_check_if_tools_will_be_built)
    if(QT_FORCE_FIND_TOOLS OR (CMAKE_CROSSCOMPILING AND NOT QT_BUILD_TOOLS_WHEN_CROSSCOMPILING))
        set(will_build_tools FALSE)
    else()
        set(will_build_tools TRUE)
    endif()
    set(QT_WILL_BUILD_TOOLS ${will_build_tools} CACHE INTERNAL "Are tools going to be built" FORCE)
endfunction()

# Wrapper function to create a regular cmake target and forward all the
# arguments collected by the conversion script. This is only meant for tests!
function(qt_add_cmake_library target)
    # Process arguments:
    qt_parse_all_arguments(arg "qt_add_cmake_library"
        "SHARED;MODULE;STATIC;INTERFACE"
        "OUTPUT_DIRECTORY;ARCHIVE_INSTALL_DIRECTORY;INSTALL_DIRECTORY"
        "${__default_private_args};${__default_public_args}"
        ${ARGN}
    )

    ### Define Targets:
    if(${arg_INTERFACE})
        add_library("${target}" INTERFACE)
    elseif(${arg_STATIC} OR (${arg_MODULE} AND NOT BUILD_SHARED_LIBS))
        add_library("${target}" STATIC)
    elseif(${arg_SHARED})
        add_library("${target}" SHARED)
    elseif(${arg_MODULE})
        add_library("${target}" MODULE)
        set_property(TARGET ${name} PROPERTY C_VISIBILITY_PRESET default)
        set_property(TARGET ${name} PROPERTY CXX_VISIBILITY_PRESET default)

        if(APPLE)
            # CMake defaults to using .so extensions for loadable modules, aka plugins,
            # but Qt plugins are actually suffixed with .dylib.
            set_property(TARGET "${target}" PROPERTY SUFFIX ".dylib")
        endif()
        if(WIN32)
            # CMake sets for Windows-GNU platforms the suffix "lib"
            set_property(TARGET "${target}" PROPERTY PREFIX "")
        endif()
    else()
        add_library("${target}")
    endif()

    if (NOT arg_ARCHIVE_INSTALL_DIRECTORY AND arg_INSTALL_DIRECTORY)
        set(arg_ARCHIVE_INSTALL_DIRECTORY "${arg_INSTALL_DIRECTORY}")
    endif()

    if (ANDROID)
        qt_android_apply_arch_suffix("${target}")
    endif()
    qt_skip_warnings_are_errors_when_repo_unclean("${target}")

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

    qt_extend_target("${target}"
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

#
# This function replaces qmake's qt_helper_lib feature. It is intended to
# compile 3rdparty libraries as part of the build.
#
function(qt_add_3rdparty_library target)
    # Process arguments:
    qt_parse_all_arguments(arg "qt_add_3rdparty_library"
        "SHARED;MODULE;STATIC;INTERFACE;EXCEPTIONS;INSTALL;SKIP_AUTOMOC"
        "OUTPUT_DIRECTORY;QMAKE_LIB_NAME"
        "${__default_private_args};${__default_public_args}"
        ${ARGN}
    )

    ### Define Targets:
    if(${arg_INTERFACE})
        add_library("${target}" INTERFACE)
    elseif(${arg_STATIC} OR (${arg_MODULE} AND NOT BUILD_SHARED_LIBS))
        add_library("${target}" STATIC)
    elseif(${arg_SHARED})
        add_library("${target}" SHARED)
    elseif(${arg_MODULE})
        add_library("${target}" MODULE)
        set_property(TARGET ${name} PROPERTY C_VISIBILITY_PRESET default)
        set_property(TARGET ${name} PROPERTY CXX_VISIBILITY_PRESET default)

        if(APPLE)
            # CMake defaults to using .so extensions for loadable modules, aka plugins,
            # but Qt plugins are actually suffixed with .dylib.
            set_property(TARGET "${target}" PROPERTY SUFFIX ".dylib")
        endif()
    else()
        add_library("${target}")
    endif()

    if(NOT arg_INTERFACE)
        qt_set_common_target_properties(${target})
    endif()

    if (NOT arg_ARCHIVE_INSTALL_DIRECTORY AND arg_INSTALL_DIRECTORY)
        set(arg_ARCHIVE_INSTALL_DIRECTORY "${arg_INSTALL_DIRECTORY}")
    endif()

    qt_internal_add_qt_repo_known_module(${target})
    qt_internal_add_target_aliases(${target})

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

    if(NOT arg_INTERFACE)
        # This property is used for super builds with static libraries. We use
        # it in QtPlugins.cmake.in to avoid "polluting" the dependency chain
        # for the target in it's project directory.
        # E.g: When we process find_package(Qt6 ... Gui) in QtDeclarative, the
        # rules in QtPugins.cmake add all the known Gui plugins as interface
        # dependencies. This in turn causes circular dependencies on every
        # plugin which links against Gui. Plugin A -> GUI -> Plugin A ....
        set_target_properties(${target} PROPERTIES QT_BUILD_PROJECT_NAME ${PROJECT_NAME})
    endif()

    if(NOT arg_EXCEPTIONS AND NOT arg_INTERFACE)
        qt_internal_set_no_exceptions_flags("${target}")
    endif()

    qt_generate_3rdparty_lib_pri_file("${target}" "${arg_QMAKE_LIB_NAME}" pri_file)
    if(pri_file)
        qt_install(FILES "${pri_file}" DESTINATION "${INSTALL_MKSPECSDIR}/modules")
    endif()

    qt_extend_target("${target}"
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
        LIBRARIES ${arg_LIBRARIES}
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
        set(path_suffix "${INSTALL_CMAKE_NAMESPACE}${target}")
        qt_path_join(config_build_dir ${QT_CONFIG_BUILD_DIR} ${path_suffix})
        qt_path_join(config_install_dir ${QT_CONFIG_INSTALL_DIR} ${path_suffix})
        set(export_name "${INSTALL_CMAKE_NAMESPACE}${target}Targets")

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

        qt_internal_export_modern_cmake_config_targets_file(
            TARGETS ${target}
            EXPORT_NAME_PREFIX ${INSTALL_CMAKE_NAMESPACE}${target}
            CONFIG_INSTALL_DIR "${config_install_dir}"
        )
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

function(qt_get_tool_cmake_configuration out_var)
    qt_get_main_cmake_configuration("${out_var}")
    string(TOUPPER "${${out_var}}" upper_config)
    set("${out_var}" "${upper_config}" PARENT_SCOPE)
endfunction()

function(qt_get_main_cmake_configuration out_var)
    if(CMAKE_BUILD_TYPE)
        set(config "${CMAKE_BUILD_TYPE}")
    elseif(QT_MULTI_CONFIG_FIRST_CONFIG)
        set(config "${QT_MULTI_CONFIG_FIRST_CONFIG}")
    endif()
    set("${out_var}" "${config}" PARENT_SCOPE)
endfunction()

# Returns the target name for the tool with the given name.
#
# In most cases, the target name is the same as the tool name.
# If the user specifies to build tools when cross-compiling, then the
# suffix "_native" is appended.
function(qt_get_tool_target_name out_var name)
    if (CMAKE_CROSSCOMPILING AND QT_BUILD_TOOLS_WHEN_CROSSCOMPILING)
        set(${out_var} ${name}_native PARENT_SCOPE)
    else()
        set(${out_var} ${name} PARENT_SCOPE)
    endif()
endfunction()

# Returns the tool name for a given tool target.
# This is the inverse of qt_get_tool_target_name.
function(qt_tool_target_to_name out_var target)
    set(name ${target})
    if (CMAKE_CROSSCOMPILING AND QT_BUILD_TOOLS_WHEN_CROSSCOMPILING)
        string(REGEX REPLACE "_native$" "" name ${target})
    endif()
    set(${out_var} ${name} PARENT_SCOPE)
endfunction()

# This function is used to define a "Qt tool", such as moc, uic or rcc.
# The BOOTSTRAP option allows building it as standalone program, otherwise
# it will be linked against QtCore.
#
# We must pass this function a target name obtained from
# qt_get_tool_target_name like this:
#     qt_get_tool_target_name(target_name my_tool)
#     qt_add_tool(${target_name})
#
function(qt_add_tool target_name)
    qt_tool_target_to_name(name ${target_name})
    qt_parse_all_arguments(arg "qt_add_tool" "BOOTSTRAP;NO_QT;NO_INSTALL"
                               "TOOLS_TARGET;${__default_target_info_args}"
                               "${__default_private_args}" ${ARGN})

    # Handle case when a tool does not belong to a module and it can't be built either (like
    # during a cross-compile).
    if(NOT arg_TOOLS_TARGET AND NOT QT_WILL_BUILD_TOOLS)
        message(FATAL_ERROR "The tool \"${name}\" has not been assigned to a module via"
                            " TOOLS_TARGET (so it can't be found) and it can't be built"
                            " (QT_WILL_BUILD_TOOLS is ${QT_WILL_BUILD_TOOLS}).")
    endif()

    if(CMAKE_CROSSCOMPILING AND QT_BUILD_TOOLS_WHEN_CROSSCOMPILING AND (name STREQUAL target_name))
        message(FATAL_ERROR
            "qt_add_tool must be passed a target obtained from qt_get_tool_target_name.")
    endif()

    set(full_name "${QT_CMAKE_EXPORT_NAMESPACE}::${name}")
    set(imported_tool_target_found FALSE)
    if(TARGET ${full_name})
        get_property(path TARGET ${full_name} PROPERTY LOCATION)
        message(STATUS "Tool '${full_name}' was found at ${path}.")
        set(imported_tool_target_found TRUE)
        if(CMAKE_CROSSCOMPILING AND NOT QT_BUILD_TOOLS_WHEN_CROSSCOMPILING)
            return()
        endif()
    endif()

    if(arg_TOOLS_TARGET AND (NOT QT_WILL_BUILD_TOOLS OR QT_BUILD_TOOLS_WHEN_CROSSCOMPILING)
            AND NOT imported_tool_target_found)
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
            if (NOT QT_BUILD_TOOLS_WHEN_CROSSCOMPILING)
                return()
            endif()
        endif()
    endif()

    if(NOT QT_WILL_BUILD_TOOLS)
        message(FATAL_ERROR "The tool \"${full_name}\" was not found in the "
                           "${tools_package_name} package. "
                           "Package found: ${${tools_package_name}_FOUND}")
    else()
        if(QT_BUILD_TOOLS_WHEN_CROSSCOMPILING)
            message(STATUS "Tool '${target_name}' will be cross-built from source.")
        else()
            message(STATUS "Tool '${full_name}' will be built from source.")
        endif()
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

    qt_add_executable("${target_name}" OUTPUT_DIRECTORY "${QT_BUILD_DIR}/${INSTALL_BINDIR}"
        ${bootstrap}
        ${no_qt}
        NO_INSTALL
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
        TARGET_VERSION "${arg_TARGET_VERSION}"
        TARGET_PRODUCT "${arg_TARGET_PRODUCT}"
        TARGET_DESCRIPTION "${arg_TARGET_DESCRIPTION}"
        TARGET_COMPANY "${arg_TARGET_COMPANY}"
        TARGET_COPYRIGHT "${arg_TARGET_COPYRIGHT}"
    )
    qt_internal_add_target_aliases("${target_name}")

    if (NOT target_name STREQUAL name)
        set_target_properties(${target_name} PROPERTIES
            OUTPUT_NAME ${name}
            EXPORT_NAME ${name}
        )
    endif()

    # If building with a multi-config configuration, the main configuration tool will be placed in
    # ./bin, while the rest will be in <CONFIG> specific subdirectories.
    qt_get_tool_cmake_configuration(tool_cmake_configuration)
    set_target_properties("${target_name}" PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${tool_cmake_configuration} "${QT_BUILD_DIR}/${INSTALL_BINDIR}"
    )

    if(NOT arg_NO_INSTALL AND arg_TOOLS_TARGET)
        # Assign a tool to an export set, and mark the module to which the tool belongs.
        qt_internal_append_known_modules_with_tools("${arg_TOOLS_TARGET}")

        # Also append the tool to the module list.
        qt_internal_append_known_module_tool("${arg_TOOLS_TARGET}" "${target_name}")

        qt_get_cmake_configurations(cmake_configs)

        set(install_initial_call_args
            EXPORT "${INSTALL_CMAKE_NAMESPACE}${arg_TOOLS_TARGET}ToolsTargets")

        foreach(cmake_config ${cmake_configs})
            qt_get_install_target_default_args(
                OUT_VAR install_targets_default_args
                CMAKE_CONFIG "${cmake_config}"
                ALL_CMAKE_CONFIGS "${cmake_configs}")
            qt_install(TARGETS "${target_name}"
                       ${install_initial_call_args}
                       CONFIGURATIONS ${cmake_config}
                       ${install_targets_default_args})
            unset(install_initial_call_args)
        endforeach()

        qt_apply_rpaths(TARGET "${target_name}" INSTALL_PATH "${INSTALL_BINDIR}" RELATIVE_RPATH)

    endif()

    if(QT_FEATURE_separate_debug_info AND (UNIX OR MINGW))
        qt_enable_separate_debug_info(${target_name} ${INSTALL_BINDIR})
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
function(qt_add_simd_part target)
    qt_parse_all_arguments(arg "qt_add_simd_part" "" ""
       "NAME;SIMD;${__default_private_args};COMPILE_FLAGS" ${ARGN})
    if ("x${arg_SIMD}" STREQUAL x)
        message(FATAL_ERROR "qt_add_simd_part needs a SIMD type to be set.")
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
            message("qt_add_simd_part(${target} SIMD ${arg_SIMD} ...): Evaluated")
        endif()
        string(TOUPPER "QT_CFLAGS_${arg_SIMD}" simd_flags)

        foreach(source IN LISTS arg_SOURCES)
            set_property(SOURCE "${source}" APPEND
                PROPERTY COMPILE_OPTIONS
                ${${simd_flags}}
                ${arg_COMPILE_FLAGS}
            )
        endforeach()
        set_source_files_properties(${arg_SOURCES} PROPERTIES SKIP_PRECOMPILE_HEADERS TRUE)
        target_sources(${target} PRIVATE ${arg_SOURCES})
    else()
        if(QT_CMAKE_DEBUG_EXTEND_TARGET)
            message("qt_add_simd_part(${target} SIMD ${arg_SIMD} ...): Skipped")
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
    cmake_parse_arguments(arg "" "OUTPUT_MOC_JSON_FILES" "FLAGS" ${ARGN})
    set(moc_files)
    set(metatypes_json_list)
    foreach(infile ${arg_UNPARSED_ARGUMENTS})
        qt_make_output_file("${infile}" "moc_" ".cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}" outfile)
        list(APPEND moc_files "${outfile}")

        set(moc_parameters_file "${outfile}_parameters$<$<BOOL:$<CONFIGURATION>>:_$<CONFIGURATION>>")
        set(moc_parameters ${arg_FLAGS} -o "${outfile}" "${infile}")

        set(metatypes_byproducts)
        if (arg_OUTPUT_MOC_JSON_FILES)
            set(moc_json_file "${outfile}.json")
            list(APPEND moc_parameters --output-json)
            list(APPEND metatypes_json_list "${outfile}.json")
            set(metatypes_byproducts "${outfile}.json")
        endif()

        string (REPLACE ";" "\n" moc_parameters "${moc_parameters}")

        file(GENERATE OUTPUT "${moc_parameters_file}" CONTENT "${moc_parameters}\n")

        add_custom_command(OUTPUT "${outfile}" ${metatypes_byproducts}
                           COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::moc "@${moc_parameters_file}"
                           DEPENDS "${infile}" ${moc_depends} ${QT_CMAKE_EXPORT_NAMESPACE}::moc
                           WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" VERBATIM)
    endforeach()
    set("${result}" ${moc_files} PARENT_SCOPE)

    # Register generated json files
    if (arg_OUTPUT_MOC_JSON_FILES)
        set(${arg_OUTPUT_MOC_JSON_FILES} "${metatypes_json_list}" PARENT_SCOPE)
    endif()
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
                       DEPENDS "${absolute_in_file_path}" ${QT_CMAKE_EXPORT_NAMESPACE}::qdbusxml2cpp
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
    file(RELATIVE_PATH relpath "${PROJECT_BINARY_DIR}" "${source_absolute_path}")

    if (arg_PRIVATE)
        set(fwd "${PROJECT_VERSION}/${module}/private/${file_name}")
    else()
        set(fwd "${file_name}")
    endif()

    string(APPEND ${arg_OUT_VAR} " ${relpath}:${fwd}")
    set(${arg_OUT_VAR} ${${arg_OUT_VAR}} PARENT_SCOPE)
endfunction()


function(qt_add_docs)
    if(${ARGC} EQUAL 1)
        # Function called from old generated CMakeLists.txt that was missing the target parameter
        return()
    endif()
    if(NOT ${ARGC} EQUAL 2)
        message(FATAL_ERROR "qt_add_docs called with the wrong number of arguments. Should be qt_add_docs(target path_to_project.qdocconf).")
        return()
    endif()
    set(target ${ARGV0})
    set(doc_project ${ARGV1})

    # If a target is not built (which can happen for tools when crosscompiling, we shouldn't try
    # to generate docs.
    if(NOT TARGET "${target}")
        return()
    endif()

    if (NOT QT_SUPERBUILD OR QT_WILL_INSTALL)
        set(qdoc_bin "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${INSTALL_BINDIR}/qdoc")
        set(qtattributionsscanner_bin "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${INSTALL_BINDIR}/qtattributionsscanner")
        set(qhelpgenerator_bin "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${INSTALL_BINDIR}/qhelpgenerator")
    else()
        set(qdoc_bin "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/qtbase/${INSTALL_BINDIR}/qdoc")
        set(qtattributionsscanner_bin "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/qtbase/${INSTALL_BINDIR}/qtattributionsscanner")
        set(qhelpgenerator_bin "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/qtbase/${INSTALL_BINDIR}/qhelpgenerator")
    endif()

    get_target_property(target_type ${target} TYPE)
    if (NOT target_type STREQUAL "INTERFACE_LIBRARY")
        get_target_property(target_bin_dir ${target} BINARY_DIR)
        get_target_property(target_source_dir ${target} SOURCE_DIR)
    else()
        set(target_bin_dir ${CMAKE_CURRENT_BINARY_DIR})
        set(target_source_dir ${CMAKE_CURRENT_SOURCE_DIR})
    endif()
    set(doc_ouput_dir "${target_bin_dir}/.doc")


    # Generate include dir list
    set(target_include_dirs_file "${doc_ouput_dir}/$<CONFIG>/includes.txt")

    set(include_paths_property "$<TARGET_PROPERTY:${target},INCLUDE_DIRECTORIES>")
    if (NOT target_type STREQUAL "INTERFACE_LIBRARY")
        file(GENERATE
            OUTPUT ${target_include_dirs_file}
            CONTENT "-I$<JOIN:${include_paths_property},\n-I>"
        )
        set(include_path_args "@${target_include_dirs_file}")
    else()
        set(include_path_args "")
    endif()

    get_filename_component(doc_target "${doc_project}" NAME_WLE)
    if (QT_WILL_INSTALL)
        set(qdoc_output_dir "${CMAKE_BINARY_DIR}/${INSTALL_DOCDIR}/${doc_target}")
        set(index_dir "${CMAKE_BINARY_DIR}/${INSTALL_DOCDIR}")
    elseif (QT_SUPERBUILD)
        set(qdoc_output_dir "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/qtbase/${INSTALL_DOCDIR}/${doc_target}")
        set(index_dir "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/qtbase/${INSTALL_DOCDIR}")
    else()
        set(qdoc_output_dir "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${INSTALL_DOCDIR}/${doc_target}")
        set(index_dir "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${INSTALL_DOCDIR}")
    endif()

    # qtattributionsscanner
    add_custom_target(qattributionsscanner_${target}
        DEPENDS ${qattributionsscanner_bin}
        COMMAND ${qtattributionsscanner_bin}
        ${PROJECT_SOURCE_DIR}
        --filter "QDocModule=${qdoc_target}"
        -o "${target_bin_dir}/codeattributions.qdoc"
    )

    # prepare docs target
    set(prepare_qdoc_args
        -outputdir "${qdoc_output_dir}"
        -installdir "${QT_INSTALL_DIR}/${INSTALL_DOCDIR}"
        "${target_source_dir}/${doc_project}"
        -prepare
        -indexdir "${index_dir}"
        -no-link-errors
        "${include_path_args}"
    )

    if (QT_WILL_INSTALL)
        set(qt_install_docs_env "${CMAKE_INSTALL_PREFIX}/${INSTALL_DOCDIR}")
    elseif (QT_SUPERBUILD)
        set(qt_install_docs_env "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/qtbase/${INSTALL_DOCDIR}")
    else()
        set(qt_install_docs_env "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${INSTALL_DOCDIR}")
    endif()

    set(qdoc_env_args
        "QT_INSTALL_DOCS=\"${qt_install_docs_env}\""
        "QT_VERSION=${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
        "QT_VER=${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
        "QT_VERSION_TAG=${PROJECT_VERSION_MAJOR}${PROJECT_VERSION_MINOR}${PROJECT_VERSION_PATCH}"
        "BUILDDIR=${target_bin_dir}"
    )

    add_custom_target(prepare_docs_${target}
        DEPENDS ${qdoc_bin}
        COMMAND ${CMAKE_COMMAND} -E env ${qdoc_env_args}
        ${qdoc_bin}
        ${prepare_qdoc_args}
    )

    add_dependencies(prepare_docs_${target} qattributionsscanner_${target})

    # generate docs target
    set(generate_qdocs_args
        -outputdir "${qdoc_output_dir}"
        -installdir "${INSTALL_DOCDIR}"
        "${target_source_dir}/${doc_project}"
        -generate
        -indexdir "${index_dir}"
        "${include_path_args}"
    )

    add_custom_target(generate_docs_${target}
        DEPENDS ${qdoc_bin}
        COMMAND ${CMAKE_COMMAND} -E env ${qdoc_env_args}
        ${qdoc_bin}
        ${generate_qdocs_args}
    )

    add_dependencies(generate_docs_${target} prepare_docs_${target})

    # generate html
    set(html_qdocs_args
        -outputdir "${qdoc_output_dir}"
        -installdir "${INSTALL_DOCDIR}"
        "${target_source_dir}/${doc_project}"
        -indexdir "${index_dir}"
        "${include_path_args}"
    )

    add_custom_target(html_docs_${target}
        DEPENDS ${qdoc_bin}
        COMMAND ${CMAKE_COMMAND} -E env ${qdoc_env_args}
        ${qdoc_bin}
        ${html_qdocs_args}
    )

    add_dependencies(html_docs_${target} generate_docs_${target})

    # generate .qch
    set(qch_file_name ${doc_target}.qch)
    set(qch_file_path ${qdoc_output_dir}/${qch_file_name})

    add_custom_target(qch_docs_${target}
        DEPENDS ${qhelpgenerator_bin}
        COMMAND ${qhelpgenerator_bin}
           "${qdoc_output_dir}/${doc_target}.qhp"
           -o "${qch_file_path}"
    )
    add_dependencies(qch_docs_${target} generate_docs_${target})

    if (QT_WILL_INSTALL)
        add_custom_target(install_html_docs_${target}
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${qdoc_output_dir}"
                "${CMAKE_INSTALL_PREFIX}/${INSTALL_DOCDIR}/${doc_target}"
            COMMENT "Installing html docs for target ${target}"
        )

        add_custom_target(install_qch_docs_${target}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${qch_file_path}"
                "${CMAKE_INSTALL_PREFIX}/${INSTALL_DOCDIR}/${qch_file_name}"
            COMMENT "Installing qch docs for target ${target}"
       )
    else()
        # Don't need to do anything when not installing
        add_custom_target(install_html_docs_${target})
        add_custom_target(install_qch_docs_${target})
    endif()

    add_dependencies(install_html_docs_${target} html_docs_${target})
    add_dependencies(install_qch_docs_${target} qch_docs_${target})

    add_custom_target(install_docs_${target})
    add_dependencies(install_docs_${target} install_html_docs_${target} install_qch_docs_${target})

    add_custom_target(docs_${target})
    add_dependencies(docs_${target} html_docs_${target})
    add_dependencies(docs_${target} qch_docs_${target})

    add_dependencies(${qt_docs_prepare_target_name} prepare_docs_${target})
    add_dependencies(${qt_docs_generate_target_name} generate_docs_${target})
    add_dependencies(${qt_docs_html_target_name} html_docs_${target})
    add_dependencies(${qt_docs_qch_target_name} qch_docs_${target})
    add_dependencies(${qt_docs_target_name} docs_${target})
    add_dependencies(${qt_docs_install_html_target_name} install_html_docs_${target})
    add_dependencies(${qt_docs_install_qch_target_name} install_qch_docs_${target})
    add_dependencies(${qt_docs_install_target_name} install_docs_${target})

endfunction()

macro(qt_find_package)
    # Get the target names we expect to be provided by the package.
    set(options CONFIG NO_MODULE MODULE REQUIRED)
    set(oneValueArgs MODULE_NAME QMAKE_LIB)
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

        if(arg_MODULE_NAME AND arg_QMAKE_LIB
           AND (NOT arg_QMAKE_LIB IN_LIST QT_QMAKE_LIBS_FOR_${arg_MODULE_NAME}))
            set(QT_QMAKE_LIBS_FOR_${arg_MODULE_NAME}
                ${QT_QMAKE_LIBS_FOR_${arg_MODULE_NAME}};${arg_QMAKE_LIB} CACHE INTERNAL "")
            set(QT_QMAKE_LIB_TARGETS_${arg_QMAKE_LIB} ${arg_PROVIDED_TARGETS} CACHE INTERNAL "")
        endif()
    endif()
endmacro()

macro(qt_add_qmake_lib_dependency lib dep)
    string(REPLACE "-" "_" dep ${dep})
    string(TOUPPER "${dep}" ucdep)
    list(APPEND QT_QMAKE_LIB_DEPS_${lib} ${ucdep})
endmacro()

macro(qt_find_apple_system_frameworks)
    if(APPLE)
        find_library(FWAppKit AppKit)
        find_library(FWAssetsLibrary AssetsLibrary)
        find_library(FWAudioToolbox AudioToolbox)
        find_library(FWApplicationServices ApplicationServices)
        find_library(FWCarbon Carbon)
        find_library(FWCoreFoundation CoreFoundation)
        find_library(FWCoreServices CoreServices)
        find_library(FWCoreGraphics CoreGraphics)
        find_library(FWCoreText CoreText)
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
            DEPENDS ${QT_CMAKE_EXPORT_NAMESPACE}::qlalr
            MAIN_DEPENDENCY ${input_file}
        )
        target_sources(${consuming_target} PRIVATE ${cpp_file} ${impl_file})
    endforeach()
endfunction()


# qt_configure_file(OUTPUT output-file <INPUT input-file | CONTENT content>)
# input-file is relative to ${CMAKE_CURRENT_SOURCE_DIR}
# output-file is relative to ${CMAKE_CURRENT_BINARY_DIR}
#
# This function is similar to file(GENERATE OUTPUT) except it writes the content
# to the file at configure time, rather than at generate time. Once CMake 3.18 is released, it can use file(CONFIGURE) in its implmenetation. Until then, it
# uses configure_file() with a generic input file as source, when used with the CONTENT signature.
function(qt_configure_file)
    qt_parse_all_arguments(arg "qt_configure_file" "" "OUTPUT;INPUT;CONTENT" "" ${ARGN})

    if(NOT arg_OUTPUT)
        message(FATAL_ERROR "No output file provided to qt_configure_file.")
    endif()

    if(arg_CONTENT)
        set(template_name "QtFileConfigure.txt.in")
        # When building qtbase, use the source template file.
        # Otherwise use the installed file.
        # This should work for non-prefix and superbuilds as well.
        if(QtBase_SOURCE_DIR)
            set(input_file "${QtBase_SOURCE_DIR}/cmake/${template_name}")
        else()
            set(input_file "${Qt6_DIR}/${template_name}")
        endif()
        set(__qt_file_configure_content "${arg_CONTENT}")
    elseif(arg_INPUT)
        set(input_file "${arg_INPUT}")
    else()
        message(FATAL_ERROR "No input value provided to qt_configure_file.")
    endif()

    configure_file("${input_file}" "${arg_OUTPUT}" @ONLY)
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
    qt_add_string_to_qconfig_cpp("${INSTALL_BINDIR}") # TODO: Host-specific
    qt_add_string_to_qconfig_cpp("${INSTALL_LIBDIR}") # TODO: Host-specific
    qt_add_string_to_qconfig_cpp("${INSTALL_DATADIR}") # TODO: Host-specific
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
        set(lib_location_absolute_path
            "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${INSTALL_BINDIR}")
    else()
        set(lib_location_absolute_path
            "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${INSTALL_LIBDIR}")
    endif()
    file(RELATIVE_PATH from_lib_location_to_prefix
         "${lib_location_absolute_path}" "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}")

    if(QT_HOST_PATH)
        set(host_prefix "${QT_HOST_PATH}")
        set(host_bin_dir_absolute_path "${QT_HOST_PATH}/${INSTALL_BINDIR}")
    else()
        set(host_prefix "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}")
        set(host_bin_dir_absolute_path
            "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${INSTALL_BINDIR}")
    endif()

    file(RELATIVE_PATH from_host_bin_dir_to_host_prefix
         "${host_bin_dir_absolute_path}" "${host_prefix}")

    # TODO: Fix this to use the equivalent of extprefix on CMake (CMAKE_STAGING_PREFIX?)
    # For now just assume ext prefix is same as regular prefix.
    file(RELATIVE_PATH from_host_bin_dir_to_ext_prefix
         "${host_bin_dir_absolute_path}" "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}")


    set(QT_CONFIGURE_LIBLOCATION_TO_PREFIX_PATH "${from_lib_location_to_prefix}")
    set(QT_CONFIGURE_HOSTBINDIR_TO_HOSTPREFIX_PATH "${from_host_bin_dir_to_host_prefix}")
    set(QT_CONFIGURE_HOSTBINDIR_TO_EXTPREFIX_PATH "${from_host_bin_dir_to_ext_prefix}")

    configure_file(global/qconfig.cpp.in global/qconfig.cpp @ONLY)
endfunction()

function(qt_set_language_standards)
    ## Use the latest standard the compiler supports (same as qt_common.prf)
    if (QT_FEATURE_cxx2a)
        set(CMAKE_CXX_STANDARD 20 PARENT_SCOPE)
    else()
        set(CMAKE_CXX_STANDARD 17 PARENT_SCOPE)
    endif()

    if (c_std_11 IN_LIST CMAKE_C_COMPILE_FEATURES)
        set(CMAKE_C_STANDARD 11 PARENT_SCOPE)
    elseif (c_std_99 IN_LIST CMAKE_C_COMPILE_FEATURES)
        set(CMAKE_C_STANDARD 99 PARENT_SCOPE)
    endif()
endfunction()

function(qt_set_language_standards_interface_compile_features target)
    # Regardless of which C++ standard is used to build Qt itself, require C++17 when building
    # Qt applications using CMake (because the Qt header files use C++17 features).
    set(cpp_feature "cxx_std_17")
    target_compile_features("${target}" INTERFACE ${cpp_feature})
endfunction()

function(qt_enable_msvc_cplusplus_define target visibility)
    # For MSVC we need to explicitly pass -Zc:__cplusplus to get correct __cplusplus.
    # Check qt_config_compile_test for more info.
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND MSVC_VERSION GREATER_EQUAL 1913)
        target_compile_options("${target}" ${visibility} "-Zc:__cplusplus")
    endif()
endfunction()

function(qt_enable_utf8_sources target)
    set(utf8_flags "")
    if(MSVC)
        list(APPEND utf8_flags "-utf-8")
    elseif(WIN32 AND ICC)
        list(APPEND utf8_flags "-Qoption,cpp,--unicode_source_kind,UTF-8")
    endif()

    if(utf8_flags)
        # Allow opting out by specifying the QT_NO_UTF8_SOURCE target property.
        set(genex_condition "$<NOT:$<BOOL:$<TARGET_PROPERTY:QT_NO_UTF8_SOURCE>>>")
        set(utf8_flags "$<${genex_condition}:${utf8_flags}>")
        target_compile_options("${target}" INTERFACE "${utf8_flags}")
    endif()
endfunction()

# Equivalent of qmake's qtNomakeTools(directory1 directory2).
# If QT_NO_MAKE_TOOLS is true, then targets within the given directories will be excluded from the
# default 'all' target, as well as from install phase.
# The private variable is checked by qt_add_executable.
function(qt_exclude_tool_directories_from_default_target)
    if(QT_NO_MAKE_TOOLS)
        set(absolute_path_directories "")
        foreach(directory ${ARGV})
            list(APPEND absolute_path_directories "${CMAKE_CURRENT_SOURCE_DIR}/${directory}")
        endforeach()
        set(__qt_exclude_tool_directories "${absolute_path_directories}" PARENT_SCOPE)
    endif()
endfunction()

function(qt_compute_relative_rpath_base rpath install_location out_var)
    set(install_lib_dir_absolute "${CMAKE_INSTALL_PREFIX}/${INSTALL_LIBDIR}")
    get_filename_component(rpath_absolute "${rpath}"
                           ABSOLUTE BASE_DIR "${install_lib_dir_absolute}")

   if(NOT IS_ABSOLUTE)
       set(install_location_absolute "${CMAKE_INSTALL_PREFIX}/${install_location}")
   endif()
    # Compute relative rpath from where the target will be installed, to the place where libraries
    # will be placed (INSTALL_LIBDIR).
    file(RELATIVE_PATH rpath_relative "${install_location_absolute}" "${rpath_absolute}")

    if("${rpath_relative}" STREQUAL "")
        # file(RELATIVE_PATH) returns an empty string if the given absolute paths are equal
        set(rpath_relative ".")
    endif()

    # Prepend $ORIGIN / @loader_path style tokens (qmake's QMAKE_REL_RPATH_BASE), to make the
    # relative rpaths work. qmake does this automatically when generating a project, so it wasn't
    # needed in the .prf files, but for CMake we need to prepend them ourselves.
    if(APPLE)
        set(rpath_rel_base "@loader_path")
    elseif(LINUX)
        set(rpath_rel_base "$ORIGIN")
    else()
        message(WARNING "No known RPATH_REL_BASE for target platform.")
        set(rpath_rel_base "NO_KNOWN_RPATH_REL_BASE")
    endif()

    if(rpath_relative STREQUAL ".")
        set(rpath_relative "${rpath_rel_base}")
    else()
        set(rpath_relative "${rpath_rel_base}/${rpath_relative}")
    endif()

    set("${out_var}" "${rpath_relative}" PARENT_SCOPE)
endfunction()

# Applies necessary rpaths to a target upon target installation.
# No-op when targeting Windows, Android, or non-prefix builds.
#
# If no RELATIVE_RPATH option is given, embeds an absolute path rpath to ${INSTALL_LIBDIR}.
# If RELATIVE_RPATH is given, the INSTALL_PATH value is to compute the relative path from
# ${INSTALL_LIBDIR} to wherever the target will be installed (the value of INSTALL_PATH).
# It's the equivalent of qmake's relative_qt_rpath.
# INSTALL_PATH is used to implement the equivalent of qmake's $$qtRelativeRPathBase().
#
# A cache variable QT_DISABLE_RPATH can be set to disable embedding any rpaths when installing.
function(qt_apply_rpaths)
    # No rpath support for win32 and android. Also no need to apply rpaths when doing a non-prefix
    # build.
    if(NOT QT_WILL_INSTALL OR WIN32 OR ANDROID)
        return()
    endif()

    # Rpaths xplicitly disabled (like for uikit), equivalent to qmake's no_qt_rpath.
    if(QT_DISABLE_RPATH)
        return()
    endif()

    qt_parse_all_arguments(arg "qt_apply_rpaths" "RELATIVE_RPATH" "TARGET;INSTALL_PATH" "" ${ARGN})
    if(NOT arg_TARGET)
        message(FATAL_ERRO "No target given to qt_apply_rpaths.")
    else()
        set(target "${arg_TARGET}")
    endif()

    # If a target is not built (which can happen for tools when crosscompiling, we shouldn't try
    # to apply properties.
    if(NOT TARGET "${target}")
        return()
    endif()

    # Protect against interface libraries.
    get_target_property(target_type "${target}" TYPE)
    if (target_type STREQUAL "INTERFACE_LIBRARY")
        return()
    endif()

    if(NOT arg_INSTALL_PATH)
        message(FATAL_ERROR "No INSTALL_PATH given to qt_apply_rpaths.")
    endif()

    set(rpaths "")

    # Modify the install path to contain the nested structure of a framework.
    get_target_property(is_framework "${target}" FRAMEWORK)
    if(is_framework)
        if(UIKIT)
            # Shallow framework
            string(APPEND arg_INSTALL_PATH "/Qt${target}.framework")
        else()
            # Full framework
            string(APPEND arg_INSTALL_PATH "/Qt${target}.framework/Versions/Current")
        endif()
    endif()

    # Same but for an app bundle.
    get_target_property(is_bundle "${target}" MACOSX_BUNDLE)
    if(is_bundle AND NOT is_framework)
        if(UIKIT)
            # Shallow bundle
            string(APPEND arg_INSTALL_PATH "/${target}.app")
        else()
            # Full bundle
            string(APPEND arg_INSTALL_PATH "/${target}.app/Contents/MacOS")
        endif()
    endif()

    # Somewhat similar to mkspecs/features/qt.prf
    if(arg_RELATIVE_RPATH)
        qt_compute_relative_rpath_base(
            "${_default_install_rpath}" "${arg_INSTALL_PATH}" relative_rpath)
        list(APPEND rpaths "${relative_rpath}")
    else()
        list(APPEND rpaths "${_default_install_rpath}")
    endif()

    # Somewhat similar to mkspecs/features/qt_build_extra.prf.
    foreach(rpath ${QT_EXTRA_RPATHS})
        if(IS_ABSOLUTE)
            list(APPEND rpaths "${rpath}")
        else()
            qt_compute_relative_rpath_base("${rpath}" "${arg_INSTALL_PATH}" relative_rpath)
            list(APPEND rpaths "${relative_rpath}")
        endif()
    endforeach()

    if(rpaths)
        list(REMOVE_DUPLICATES rpaths)
        set_property(TARGET "${target}" APPEND PROPERTY INSTALL_RPATH ${rpaths})
    endif()
endfunction()

# Compatibility macros that should be removed once all their usages are removed.
function(extend_target)
    qt_extend_target(${ARGV})
endfunction()

function(add_qt_module)
    qt_add_module(${ARGV})
endfunction()

function(add_qt_plugin)
    qt_add_plugin(${ARGV})
endfunction()

function(add_qt_tool)
    qt_add_tool(${ARGV})
endfunction()

function(add_qt_test)
    qt_add_test(${ARGV})
endfunction()

function(add_qt_test_helper)
    qt_add_test_helper(${ARGV})
endfunction()

function(add_qt_manual_test)
    qt_add_manual_test(${ARGV})
endfunction()

function(add_qt_benchmark)
    qt_add_benchmark(${ARGV})
endfunction()

function(add_qt_executable)
    qt_add_executable(${ARGV})
endfunction()

function(add_qt_simd_part)
    qt_add_simd_part(${ARGV})
endfunction()

function(add_qt_docs)
    qt_add_docs(${ARGV})
endfunction()

function(add_qt_resource)
    qt_add_resource(${ARGV})
endfunction()

function(add_cmake_library)
    qt_add_cmake_library(${ARGV})
endfunction()
