# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

macro(qt_internal_reset_global_state)
    qt_internal_clear_qt_repo_known_modules()
    qt_internal_clear_qt_repo_known_plugin_types()
    qt_internal_set_qt_known_plugins("")

    set(QT_KNOWN_MODULES_WITH_TOOLS "" CACHE INTERNAL "Known Qt modules with tools" FORCE)
endmacro()

macro(qt_internal_set_qt_path_separator)
    # For adjusting variables when running tests, we need to know what
    # the correct variable is for separating entries in PATH-alike
    # variables.
    if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
        set(QT_PATH_SEPARATOR "\\;")
    else()
        set(QT_PATH_SEPARATOR ":")
    endif()
endmacro()

macro(qt_internal_set_internals_extra_cmake_code)
    # This is used to hold extra cmake code that should be put into QtBuildInternalsExtra.cmake file
    # at the QtPostProcess stage.
    set(QT_BUILD_INTERNALS_EXTRA_CMAKE_CODE "")
endmacro()

macro(qt_internal_set_top_level_source_dir)
    # Save the value of the current first project source dir.
    # This will be /path/to/qtbase for qtbase both in a super-build and a non super-build.
    # This will be /path/to/qtbase/tests when building standalone tests.
    set(QT_TOP_LEVEL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endmacro()

macro(qt_internal_set_apple_archiver_flags)
    # Prevent warnings about object files without any symbols. This is a common
    # thing in Qt as we tend to build files unconditionally, and then use ifdefs
    # to compile out parts that are not relevant.
    if(CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
        foreach(lang ASM C CXX)
            # We have to tell 'ar' to not run ranlib by itself, by passing the 'S' option
            set(CMAKE_${lang}_ARCHIVE_CREATE "<CMAKE_AR> qcS <TARGET> <LINK_FLAGS> <OBJECTS>")
            set(CMAKE_${lang}_ARCHIVE_APPEND "<CMAKE_AR> qS <TARGET> <LINK_FLAGS> <OBJECTS>")
            set(CMAKE_${lang}_ARCHIVE_FINISH "<CMAKE_RANLIB> -no_warning_for_no_symbols <TARGET>")
        endforeach()
    endif()
endmacro()

macro(qt_internal_set_debug_extend_target)
    option(QT_CMAKE_DEBUG_EXTEND_TARGET "Debug extend_target calls in Qt's build system" OFF)
endmacro()

macro(qt_internal_include_all_helpers)
    # Upstream cmake modules.
    include(CheckCXXSourceCompiles)
    include(CMakePackageConfigHelpers)

    # Internal helpers available only while building Qt itself.
    include(Qt3rdPartyLibraryHelpers)
    include(QtAppHelpers)
    include(QtAutogenHelpers)
    include(QtBuildPathsHelpers)
    include(QtCMakeHelpers)
    include(QtDbusHelpers)
    include(QtDeferredDependenciesHelpers)
    include(QtDocsHelpers)
    include(QtExecutableHelpers)
    include(QtFindPackageHelpers)
    include(QtFlagHandlingHelpers)
    include(QtFrameworkHelpers)
    include(QtGlobalStateHelpers)
    include(QtHeadersClean)
    include(QtInstallHelpers)
    include(QtJavaHelpers)
    include(QtLalrHelpers)
    include(QtMkspecHelpers)
    include(QtModuleHelpers)
    include(QtNoLinkTargetHelpers)
    include(QtPkgConfigHelpers)
    include(QtPluginHelpers)
    include(QtPrecompiledHeadersHelpers)
    include(QtPriHelpers)
    include(QtPrlHelpers)
    include(QtQmakeHelpers)
    include(QtResourceHelpers)
    include(QtRpathHelpers)
    include(QtSanitizerHelpers)
    include(QtScopeFinalizerHelpers)
    include(QtSeparateDebugInfo)
    include(QtSimdHelpers)
    include(QtSingleRepoTargetSetBuildHelpers)
    include(QtSyncQtHelpers)
    include(QtTargetHelpers)
    include(QtTestHelpers)
    include(QtToolHelpers)
    include(QtUnityBuildHelpers)

    if(ANDROID)
        include(QtAndroidHelpers)
    endif()

    if(WASM)
        include(QtWasmHelpers)
    endif()

    # Helpers that are available in public projects and while building Qt itself.
    include(QtPublicAppleHelpers)
    include(QtPublicCMakeHelpers)
    include(QtPublicDependencyHelpers)
    include(QtPublicExternalProjectHelpers)
    include(QtPublicFindPackageHelpers)
    include(QtPublicPluginHelpers)
    include(QtPublicTargetHelpers)
    include(QtPublicTestHelpers)
    include(QtPublicToolHelpers)
    include(QtPublicWalkLibsHelpers)
endmacro()

function(qt_internal_check_host_path_set_for_cross_compiling)
    if(CMAKE_CROSSCOMPILING)
        if(NOT IS_DIRECTORY "${QT_HOST_PATH}")
            message(FATAL_ERROR "You need to set QT_HOST_PATH to cross compile Qt.")
        endif()
    endif()
endfunction()

macro(qt_internal_setup_find_host_info_package)
    _qt_internal_determine_if_host_info_package_needed(__qt_build_requires_host_info_package)
    _qt_internal_find_host_info_package("${__qt_build_requires_host_info_package}")
endmacro()

macro(qt_internal_setup_poor_mans_scope_finalizer)
    # This sets up the poor man's scope finalizer mechanism.
    # For newer CMake versions, we use cmake_language(DEFER CALL) instead.
    if(CMAKE_VERSION VERSION_LESS "3.19.0")
        variable_watch(CMAKE_CURRENT_LIST_DIR qt_watch_current_list_dir)
    endif()
endmacro()

macro(qt_internal_set_qt_namespace)
    set(QT_NAMESPACE "" CACHE STRING "Qt Namespace")
endmacro()

macro(qt_internal_set_qt_coord_type)
    if(PROJECT_NAME STREQUAL "QtBase")
        set(QT_COORD_TYPE double CACHE STRING "Type of qreal")
    endif()
endmacro()

function(qt_internal_check_macos_host_version)
    # macOS versions 10.14 and less don't have the implementation of std::filesystem API.
    if(CMAKE_HOST_APPLE AND CMAKE_HOST_SYSTEM_VERSION VERSION_LESS "19.0.0")
        message(FATAL_ERROR "macOS versions less than 10.15 are not supported for building Qt.")
    endif()
endfunction()

function(qt_internal_setup_tool_path_command)
    if(NOT CMAKE_HOST_WIN32)
        return()
    endif()
    set(bindir "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${INSTALL_BINDIR}")
    file(TO_NATIVE_PATH "${bindir}" bindir)
    list(APPEND command COMMAND)
    list(APPEND command set PATH=${bindir}$<SEMICOLON>%PATH%)
    set(QT_TOOL_PATH_SETUP_COMMAND "${command}" CACHE INTERNAL
        "internal command prefix for tool invocations" FORCE)
    # QT_TOOL_PATH_SETUP_COMMAND is deprecated. Please use _qt_internal_get_wrap_tool_script_path
    # instead.
endfunction()

macro(qt_internal_setup_android_platform_specifics)
    if(ANDROID)
        qt_internal_setup_android_target_properties()
    endif()
endmacro()

macro(qt_internal_setup_build_and_global_variables)
    qt_internal_setup_paths_and_prefixes()

    qt_internal_reset_global_state()

    # Depends on paths and prefixes.
    qt_internal_set_mkspecs_dir()
    qt_internal_setup_platform_definitions_and_mkspec()

    qt_internal_check_macos_host_version()
    qt_internal_check_host_path_set_for_cross_compiling()
    qt_internal_setup_android_platform_specifics()
    qt_internal_setup_find_host_info_package()
    qt_internal_setup_tool_path_command()
    qt_internal_setup_default_target_function_options()
    qt_internal_set_default_rpath_settings()
    qt_internal_set_qt_namespace()
    qt_internal_set_qt_coord_type()
    qt_internal_set_qt_path_separator()
    qt_internal_set_internals_extra_cmake_code()
    qt_internal_set_top_level_source_dir()
    qt_internal_set_apple_archiver_flags()
    qt_internal_set_debug_extend_target()
    qt_internal_setup_poor_mans_scope_finalizer()
endmacro()

