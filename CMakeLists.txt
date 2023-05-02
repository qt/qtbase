# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Need an explicit call at the top level. This is the absolute minimum version
# needed to configure the project with any combination of enabled features.
# The call to qt_build_repo_begin() will upgrade policies further.
#
# The absolute minimum version for building Qt is 3.16 because for metatype.json generation, we
# depend on being able to find the location of json files created by AUTOMOC/moc.
cmake_minimum_required(VERSION 3.16)

# Get the repo version and CMake policy details
include(.cmake.conf)

# Bail out if any part of the build directory's path is symlinked.
function(qt_internal_check_if_path_has_symlinks path)
    get_filename_component(dir "${path}" ABSOLUTE)
    set(is_symlink FALSE)
    if(CMAKE_HOST_WIN32)
        # CMake marks Windows mount points as symbolic links, so use simplified REALPATH check
        # on Windows platforms instead of IS_SYMLINK.
        get_filename_component(dir_realpath "${dir}" REALPATH)
        if(NOT dir STREQUAL dir_realpath)
            set(is_symlink TRUE)
        endif()
    else()
        while(TRUE)
            if(IS_SYMLINK "${dir}")
                set(is_symlink TRUE)
                break()
            endif()

            set(prev_dir "${dir}")
            get_filename_component(dir "${dir}" DIRECTORY)
            if("${dir}" STREQUAL "${prev_dir}")
                return()
            endif()
        endwhile()
    endif()
    if(is_symlink)
        set(possible_solutions_for_resolving_symlink [[
    - Map directories using a transparent mechanism such as mount --bind
    - Pass the real path of the build directory to CMake, e.g. using
      cd $(realpath <path>) before invoking cmake <source_dir>.
            ]])
        if(QT_ALLOW_SYMLINK_IN_PATHS)
            # In some cases, e.g., Homebrew, it is beneficial to skip this check.
            # Before this, Homebrew had to patch this out to be able to get their build.
            message(WARNING
                "The path \"${path}\" contains symlinks. "
                "This is not recommended, and it may lead to unexpected issues. If you do "
                "not have a good reason for enabling 'QT_ALLOW_SYMLINK_IN_PATHS', disable "
                "it, and follow one of the following solutions: \n"
                "${possible_solutions_for_resolving_symlink} ")
        else()
            message(FATAL_ERROR
                "The path \"${path}\" contains symlinks. "
                "This is not supported. Possible solutions: \n"
                "${possible_solutions_for_resolving_symlink} ")
        endif()
    endif()
endfunction()
qt_internal_check_if_path_has_symlinks("${CMAKE_BINARY_DIR}")

# Run auto detection routines, but not when doing standalone tests. In that case, the detection
# results are taken from either QtBuildInternals or the qt.toolchain.cmake file. Also, inhibit
# auto-detection in a top-level build, because the top-level project file already includes it.
if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_SUPERBUILD)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/QtAutoDetect.cmake)
endif()

# This call will load any provided cmake toolchain file.
project(QtBase
    VERSION "${QT_REPO_MODULE_VERSION}"
    DESCRIPTION "Qt Base Libraries"
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C ASM
)

# Should this Qt be static or dynamically linked?
option(BUILD_SHARED_LIBS "Build Qt statically or dynamically" ON)
set(QT_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})

# This variable is also set in Qt6CoreConfigExtras.cmake, but it's not loaded when building
# qtbase. Set it here so qt_add_plugin can compute the proper plugin flavor.
set(QT6_IS_SHARED_LIBS_BUILD ${BUILD_SHARED_LIBS})

# BUILD_SHARED_LIBS influences the minimum required CMake version. The value is set either by:
#   a cache variable provided on the configure command line
#   or set by QtAutoDetect.cmake depending on the platform
#   or specified via a toolchain file that is loaded by the project() call
#   or set by the option() call above
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/QtCMakeVersionHelpers.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/QtPublicCMakeVersionHelpers.cmake")
qt_internal_check_and_warn_about_unsuitable_cmake_version()

if(NOT QT_BUILD_STANDALONE_TESTS)
    ## Add some paths to check for cmake modules:
    list(PREPEND CMAKE_MODULE_PATH
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/3rdparty/extra-cmake-modules/find-modules"
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/3rdparty/kwin"
    )

    if(MACOS)
        # Add module directory to pick up custom Info.plist template
        list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos")
    elseif(IOS)
        # Add module directory to pick up custom Info.plist template
        list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ios")
    endif()

    ## Find the build internals package.
    set(QT_BUILD_INTERNALS_SKIP_CMAKE_MODULE_PATH_ADDITION TRUE)
    list(PREPEND CMAKE_PREFIX_PATH
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
    )
    find_package(QtBuildInternals CMAKE_FIND_ROOT_PATH_BOTH)
    unset(QT_BUILD_INTERNALS_SKIP_CMAKE_MODULE_PATH_ADDITION)
else()
    # When building standalone tests, an istalled BuildInternals package already exists.
    find_package(Qt6 REQUIRED COMPONENTS BuildInternals CMAKE_FIND_ROOT_PATH_BOTH)
endif()

qt_internal_project_setup()

qt_build_repo_begin()

if(NOT QT_BUILD_STANDALONE_TESTS)
    ## Should this Qt be built with Werror?
    option(WARNINGS_ARE_ERRORS "Build Qt with warnings as errors" ${FEATURE_developer_build})

    ## Should this Qt create versioned hard link for some tools?
    option(QT_CREATE_VERSIONED_HARD_LINK "Enable the use of versioned hard link" ON)

    ## QtBase specific configure tests:
    include(QtBaseConfigureTests)

    ## Build System tests:
    include(QtBaseCMakeTesting)

    ## Targets for global features, etc.:
    include(QtBaseGlobalTargets)

    ## Set language standards after QtBaseGlobalTargets, because that's when the relevant
    ## feature variables are available.
    qt_set_language_standards()

    #include CoreMacros() for qt6_generate_meta_types()
    set(QT_DEFAULT_MAJOR_VERSION 6)
    include(src/corelib/Qt6CoreMacros.cmake)

    # Needed when building qtbase for android.
    if(ANDROID)
        include(src/corelib/Qt6AndroidMacros.cmake)
        _qt_internal_create_global_android_targets()
    endif()

    if(WASM)
        # Needed when building for WebAssembly.
        include(cmake/QtWasmHelpers.cmake)
        include(src/corelib/Qt6WasmMacros.cmake)
        qt_internal_setup_wasm_target_properties(Platform)
    endif()

    # Set up optimization flags like in qmake.
    # This function must be called after the global QT_FEATURE_xxx variables have been set up,
    # aka after QtBaseGlobalTargets is processed.
    # It also has to be called /before/ adding add_subdirectory(src), so that per-directory
    # modifications can still be applied if necessary (like in done in Core and Gui).
    qt_internal_set_up_config_optimizations_like_in_qmake()

    ## Setup documentation
    add_subdirectory(doc)

    ## Visit all the directories:
    add_subdirectory(src)
endif()

if(QT_BUILD_TESTS)
    add_subdirectory(tests)
    if(NOT QT_BUILD_TESTS_BY_DEFAULT)
        set_property(DIRECTORY tests PROPERTY EXCLUDE_FROM_ALL TRUE)
    endif()
endif()

if(NOT QT_BUILD_STANDALONE_TESTS)
    if(QT_WILL_BUILD_TOOLS AND QT_FEATURE_settings)
        add_subdirectory(qmake)
    endif()
    # As long as we use the mkspecs (for qplatformdefs.h), we need to always
    # install it, especially when cross-compiling.
    set(mkspecs_install_dir "${INSTALL_MKSPECSDIR}")
    qt_path_join(mkspecs_install_dir ${QT_INSTALL_DIR} ${mkspecs_install_dir})

    file(GLOB mkspecs_subdirs
        LIST_DIRECTORIES TRUE
        "${PROJECT_SOURCE_DIR}/mkspecs/*")
    foreach(entry IN LISTS mkspecs_subdirs)
        if (IS_DIRECTORY ${entry})
            qt_copy_or_install(DIRECTORY "${entry}"
                               DESTINATION ${mkspecs_install_dir}
                               USE_SOURCE_PERMISSIONS)
        else()
            qt_copy_or_install(FILES "${entry}"
                               DESTINATION ${mkspecs_install_dir})
        endif()
    endforeach()
endif()

qt_build_repo_end()

if(NOT QT_BUILD_STANDALONE_TESTS AND QT_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
