# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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
            # INSTALL_SYSCONFDIR is allowed to be outside the prefix.
            if(NOT name STREQUAL "INSTALL_SYSCONFDIR")
                message(FATAL_ERROR
                    "Path component '${name}' is outside computed install prefix: ${rel_path} ")
                return()
            endif()
            set("${name}" "${${name}}" CACHE STRING "${docstring}" FORCE)
        else()
            set("${name}" "${rel_path}" CACHE STRING "${docstring}" FORCE)
        endif()
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

if(NOT INSTALL_MKSPECSDIR MATCHES "(^|/)mkspecs")
    message(FATAL_ERROR "INSTALL_MKSPECSDIR must end with '/mkspecs'")
endif()

# Given CMAKE_CONFIG and ALL_CMAKE_CONFIGS, determines if a directory suffix needs to be appended
# to each destination, and sets the computed install target destination arguments in OUT_VAR.
# Defaults used for each of the destination types, and can be configured per destination type.
function(qt_get_install_target_default_args)
    cmake_parse_arguments(PARSE_ARGV 0 arg
       ""
       "OUT_VAR;CMAKE_CONFIG;RUNTIME;LIBRARY;ARCHIVE;INCLUDES;BUNDLE"
       "ALL_CMAKE_CONFIGS")
    _qt_internal_validate_all_args_are_parsed(arg)

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
                           "QML imports [ARCHDATADIR/qml]")
qt_configure_process_path(INSTALL_DATADIR "." "Arch-independent data [PREFIX]")
qt_configure_process_path(INSTALL_DOCDIR "${INSTALL_DATADIR}/doc" "Documentation [DATADIR/doc]")
qt_configure_process_path(INSTALL_TRANSLATIONSDIR "${INSTALL_DATADIR}/translations"
    "Translations [DATADIR/translations]")
if(APPLE)
    set(QT_DEFAULT_SYS_CONF_DIR "/Library/Preferences/Qt")
else()
    set(QT_DEFAULT_SYS_CONF_DIR "etc/xdg")
endif()
qt_configure_process_path(INSTALL_SYSCONFDIR
                          "${QT_DEFAULT_SYS_CONF_DIR}"
                          "Settings used by Qt programs [PREFIX/etc/xdg]/[/Library/Preferences/Qt]")
qt_configure_process_path(INSTALL_EXAMPLESDIR "examples" "Examples [PREFIX/examples]")
qt_configure_process_path(INSTALL_TESTSDIR "tests" "Tests [PREFIX/tests]")
qt_configure_process_path(INSTALL_DESCRIPTIONSDIR
                         "${INSTALL_ARCHDATADIR}/modules"
                          "Module description files directory")

if(NOT "${CMAKE_STAGING_PREFIX}" STREQUAL "")
    set(QT_STAGING_PREFIX "${CMAKE_STAGING_PREFIX}")
else()
    set(QT_STAGING_PREFIX "${CMAKE_INSTALL_PREFIX}")
endif()

if(PROJECT_NAME STREQUAL "QtBase")
    set(QT_COORD_TYPE double CACHE STRING "Type of qreal")
endif()

function(qt_internal_set_up_global_paths)
    # Compute the values of QT_BUILD_DIR, QT_INSTALL_DIR, QT_CONFIG_BUILD_DIR, QT_CONFIG_INSTALL_DIR
    # taking into account whether the current build is a prefix build or a non-prefix build,
    # and whether it is a superbuild or non-superbuild.
    # A third case is when another module or standalone tests are built against a super-built Qt.
    # The layout for the third case is the same as for non-superbuilds.
    #
    # These values should be prepended to file paths in commands or properties,
    # in order to correctly place generated Config files, generated Targets files,
    # executables / libraries, when copying / installing files, etc.
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
            if("${CMAKE_STAGING_PREFIX}" STREQUAL "")
                set(QT_BUILD_DIR "${QtBase_BINARY_DIR}")
                set(QT_INSTALL_DIR "${QtBase_BINARY_DIR}")
            else()
                set(QT_BUILD_DIR "${CMAKE_STAGING_PREFIX}")
                set(QT_INSTALL_DIR "${CMAKE_STAGING_PREFIX}")
            endif()
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
            set(QT_BUILD_DIR "${QT_STAGING_PREFIX}")
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

# macOS versions 10.14 and less don't have the implementation of std::filesystem API.
if(CMAKE_HOST_APPLE AND CMAKE_HOST_SYSTEM_VERSION VERSION_LESS "19.0.0")
    message(FATAL_ERROR "macOS versions less than 10.15 are not supported for building Qt.")
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

# By default, don't embed auto-determined RPATHs pointing to directories
# outside of the build tree, into the installed binaries.
# This ended up adding rpaths like ${CMAKE_INSTALL_PREFIX}/lib (or /Users/qt/work/install/lib into
# the official libraries created by the CI) into the non-qtbase libraries, plugins, etc.
#
# It should not be necessary, given that qt_apply_rpaths() already adds the necessary rpaths, either
# relocatable ones or absolute ones, depending on what the platform supports.
if(NOT QT_NO_DISABLE_CMAKE_INSTALL_RPATH_USE_LINK_PATH)
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
endif()

# Ensure that GNUInstallDirs's CMAKE_INSTALL_LIBDIR points to the same lib dir that Qt was
# configured with. Currently this is important for QML plugins, which embed an rpath based
# on that value.
set(CMAKE_INSTALL_LIBDIR "${INSTALL_LIBDIR}")

function(qt_setup_tool_path_command)
    if(NOT CMAKE_HOST_WIN32)
        return()
    endif()
    set(bindir "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${INSTALL_BINDIR}")
    file(TO_NATIVE_PATH "${bindir}" bindir)
    list(APPEND command COMMAND)
    list(APPEND command set PATH=${bindir}$<SEMICOLON>%PATH%)
    set(QT_TOOL_PATH_SETUP_COMMAND "${command}" CACHE INTERNAL "internal command prefix for tool invocations" FORCE)
    # QT_TOOL_PATH_SETUP_COMMAND is deprecated. Please use _qt_internal_get_wrap_tool_script_path
    # instead.
endfunction()
qt_setup_tool_path_command()

# Platform define path, etc.
if(WIN32)
    set(QT_DEFAULT_PLATFORM_DEFINITIONS WIN32 _ENABLE_EXTENDED_ALIGNED_STORAGE)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        list(APPEND QT_DEFAULT_PLATFORM_DEFINITIONS WIN64 _WIN64)
    endif()

    if(CLANG)
        if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC" OR MSVC)
            set(QT_DEFAULT_MKSPEC win32-clang-msvc)
        elseif(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU" OR MINGW)
            set(QT_DEFAULT_MKSPEC win32-clang-g++)
        endif()
    elseif(MSVC)
        if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
            set(QT_DEFAULT_MKSPEC win32-arm64-msvc)
        else()
            set(QT_DEFAULT_MKSPEC win32-msvc)
        endif()
    elseif(MINGW)
        set(QT_DEFAULT_MKSPEC win32-g++)
        list(APPEND QT_DEFAULT_PLATFORM_DEFINITIONS MINGW_HAS_SECURE_API=1)
    endif()
elseif(LINUX)
    if(GCC)
        set(QT_DEFAULT_MKSPEC linux-g++)
    elseif(CLANG)
        set(QT_DEFAULT_MKSPEC linux-clang)
    endif()
elseif(ANDROID)
    if(GCC)
        set(QT_DEFAULT_MKSPEC android-g++)
    elseif(CLANG)
        set(QT_DEFAULT_MKSPEC android-clang)
    endif()
elseif(IOS)
    set(QT_DEFAULT_MKSPEC macx-ios-clang)
elseif(APPLE)
    set(QT_DEFAULT_MKSPEC macx-clang)
elseif(WASM)
    if(WASM64)
        set(QT_DEFAULT_MKSPEC wasm-emscripten-64)
    else()
        set(QT_DEFAULT_MKSPEC wasm-emscripten)
    endif()
elseif(QNX)
    # Certain POSIX defines are not set if we don't compile with -std=gnuXX
    set(QT_ENABLE_CXX_EXTENSIONS ON)

    list(APPEND QT_DEFAULT_PLATFORM_DEFINITIONS _FORTIFY_SOURCE=2 _REENTRANT)

    set(compiler_aarch64le aarch64le)
    set(compiler_armle-v7 armv7le)
    set(compiler_x86-64 x86_64)
    set(compiler_x86 x86)
    foreach(arch aarch64le armle-v7 x86-64 x86)
        if (CMAKE_CXX_COMPILER_TARGET MATCHES "${compiler_${arch}}$")
            set(QT_DEFAULT_MKSPEC qnx-${arch}-qcc)
        endif()
    endforeach()
elseif(FREEBSD)
    if(CLANG)
        set(QT_DEFAULT_MKSPEC freebsd-clang)
    elseif(GCC)
        set(QT_DEFAULT_MKSPEC freebsd-g++)
    endif()
elseif(NETBSD)
    set(QT_DEFAULT_MKSPEC netbsd-g++)
elseif(OPENBSD)
    set(QT_DEFAULT_MKSPEC openbsd-g++)
elseif(SOLARIS)
    if(GCC)
        if(QT_64BIT)
             set(QT_DEFAULT_MKSPEC solaris-g++-64)
        else()
             set(QT_DEFAULT_MKSPEC solaris-g++)
        endif()
    else()
        if(QT_64BIT)
             set(QT_DEFAULT_MKSPEC solaris-cc-64)
        else()
             set(QT_DEFAULT_MKSPEC solaris-cc)
        endif()
    endif()
elseif(HURD)
    set(QT_DEFAULT_MKSPEC hurd-g++)
endif()

if(NOT QT_QMAKE_TARGET_MKSPEC)
    set(QT_QMAKE_TARGET_MKSPEC "${QT_DEFAULT_MKSPEC}" CACHE STRING "QMake target mkspec")
endif()

if(CMAKE_CROSSCOMPILING)
    set(QT_QMAKE_HOST_MKSPEC "${QT${PROJECT_VERSION_MAJOR}_HOST_INFO_QMAKE_MKSPEC}")
else()
    set(QT_QMAKE_HOST_MKSPEC "${QT_QMAKE_TARGET_MKSPEC}")
endif()

if(NOT QT_QMAKE_TARGET_MKSPEC OR NOT EXISTS "${QT_MKSPECS_DIR}/${QT_QMAKE_TARGET_MKSPEC}")
    if(NOT QT_QMAKE_TARGET_MKSPEC)
        set(reason "Platform is not detected. Please make sure your build environment is configured"
            " properly or specify it manually using QT_QMAKE_TARGET_MKSPEC variable and one of the"
            " known platforms.")
    else()
        set(reason "Unknown platform ${QT_QMAKE_TARGET_MKSPEC}")
    endif()

    file(GLOB known_platforms
        LIST_DIRECTORIES true
        RELATIVE "${QT_MKSPECS_DIR}"
        "${QT_MKSPECS_DIR}/*"
    )
    list(JOIN known_platforms "\n    " known_platforms)
    message(FATAL_ERROR "${reason}\n"
        "Known platforms:\n    ${known_platforms}")
endif()

if(NOT DEFINED QT_DEFAULT_PLATFORM_DEFINITIONS)
    set(QT_DEFAULT_PLATFORM_DEFINITIONS "")
endif()

set(QT_PLATFORM_DEFINITIONS ${QT_DEFAULT_PLATFORM_DEFINITIONS}
    CACHE STRING "Qt platform specific pre-processor defines")

set(QT_NAMESPACE "" CACHE STRING "Qt Namespace")

include(QtGlobalStateHelpers)

# Reset global state:
qt_internal_clear_qt_repo_known_modules()
qt_internal_clear_qt_repo_known_plugin_types()
qt_internal_set_qt_known_plugins("")

set(QT_KNOWN_MODULES_WITH_TOOLS "" CACHE INTERNAL "Known Qt modules with tools" FORCE)

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

# Needed for qt_internal_add_link_flags_no_undefined.
include(CheckCXXSourceCompiles)

set(__default_private_args
    SOURCES
    LIBRARIES
    INCLUDE_DIRECTORIES
    SYSTEM_INCLUDE_DIRECTORIES
    DEFINES
    DBUS_ADAPTOR_BASENAME
    DBUS_ADAPTOR_FLAGS
    DBUS_ADAPTOR_SOURCES
    DBUS_INTERFACE_BASENAME
    DBUS_INTERFACE_FLAGS
    DBUS_INTERFACE_SOURCES
    FEATURE_DEPENDENCIES
    COMPILE_OPTIONS
    LINK_OPTIONS
    MOC_OPTIONS
    DISABLE_AUTOGEN_TOOLS
    ENABLE_AUTOGEN_TOOLS
    PLUGIN_TYPES
    NO_PCH_SOURCES
    NO_UNITY_BUILD_SOURCES
)
set(__default_public_args
    PUBLIC_LIBRARIES
    PUBLIC_INCLUDE_DIRECTORIES
    PUBLIC_DEFINES
    PUBLIC_COMPILE_OPTIONS
    PUBLIC_LINK_OPTIONS
)
set(__default_private_module_args
    PRIVATE_MODULE_INTERFACE
)
set(__default_target_info_args
    TARGET_VERSION
    TARGET_PRODUCT
    TARGET_DESCRIPTION
    TARGET_COMPANY
    TARGET_COPYRIGHT
)

# Collection of arguments so they can be shared across qt_internal_add_executable
# and qt_internal_add_test_helper.
set(__qt_internal_add_executable_optional_args
    GUI
    NO_INSTALL
    EXCEPTIONS
    DELAY_RC
    DELAY_TARGET_INFO
    QT_APP
    NO_UNITY_BUILD
)
set(__qt_internal_add_executable_single_args
    CORE_LIBRARY
    OUTPUT_DIRECTORY
    INSTALL_DIRECTORY
    VERSION
    ${__default_target_info_args}
)
set(__qt_internal_add_executable_multi_args
    ${__default_private_args}
    ${__default_public_args}
)

option(QT_CMAKE_DEBUG_EXTEND_TARGET "Debug extend_target calls in Qt's build system" OFF)

# Internal helpers available only while building Qt itself.
include(Qt3rdPartyLibraryHelpers)
include(QtAppHelpers)
include(QtAutogenHelpers)
include(QtCMakeHelpers)
include(QtDeferredDependenciesHelpers)
include(QtDbusHelpers)
include(QtDocsHelpers)
include(QtExecutableHelpers)
include(QtFindPackageHelpers)
include(QtFlagHandlingHelpers)
include(QtFrameworkHelpers)
include(QtInstallHelpers)
include(QtLalrHelpers)
include(QtModuleHelpers)
include(QtNoLinkTargetHelpers)
include(QtPluginHelpers)
include(QtPrecompiledHeadersHelpers)
include(QtUnityBuildHelpers)
include(QtPkgConfigHelpers)
include(QtPriHelpers)
include(QtPrlHelpers)
include(QtQmakeHelpers)
include(QtResourceHelpers)
include(QtRpathHelpers)
include(QtSanitizerHelpers)
include(QtScopeFinalizerHelpers)
include(QtSimdHelpers)
include(QtSingleRepoTargetSetBuildHelpers)
include(QtSyncQtHelpers)
include(QtTargetHelpers)
include(QtTestHelpers)
include(QtToolHelpers)
include(QtHeadersClean)
include(QtJavaHelpers)

if(ANDROID)
    include(QtAndroidHelpers)
endif()

if(WASM)
    include(QtWasmHelpers)
endif()

# Helpers that are available in public projects and while building Qt itself.
include(QtPublicAppleHelpers)
include(QtPublicCMakeHelpers)
include(QtPublicPluginHelpers)
include(QtPublicTargetHelpers)
include(QtPublicWalkLibsHelpers)
include(QtPublicFindPackageHelpers)
include(QtPublicDependencyHelpers)
include(QtPublicTestHelpers)
include(QtPublicToolHelpers)

if(CMAKE_CROSSCOMPILING)
    if(NOT IS_DIRECTORY "${QT_HOST_PATH}")
        message(FATAL_ERROR "You need to set QT_HOST_PATH to cross compile Qt.")
    endif()
endif()

_qt_internal_determine_if_host_info_package_needed(__qt_build_requires_host_info_package)
_qt_internal_find_host_info_package("${__qt_build_requires_host_info_package}")

# Create tool script wrapper if necessary.
# TODO: Remove once all direct usages of QT_TOOL_COMMAND_WRAPPER_PATH are replaced with function
# calls.
_qt_internal_generate_tool_command_wrapper()

# This sets up the poor man's scope finalizer mechanism.
# For newer CMake versions, we use cmake_language(DEFER CALL) instead.
if(CMAKE_VERSION VERSION_LESS "3.19.0")
    variable_watch(CMAKE_CURRENT_LIST_DIR qt_watch_current_list_dir)
endif()
