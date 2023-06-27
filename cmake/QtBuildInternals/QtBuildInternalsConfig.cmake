# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# These values should be kept in sync with those in qtbase/.cmake.conf
cmake_minimum_required(VERSION 3.16...3.21)

###############################################
#
# Macros and functions for building Qt modules
#
###############################################

# Recursively reads the dependencies section from dependencies.yaml in ${repo_dir} and returns the
# list of dependencies, including transitive ones, in out_var.
#
# The returned dependencies are topologically sorted.
#
# Example output for qtdeclarative:
# qtbase;qtimageformats;qtlanguageserver;qtshadertools;qtsvg
#
function(qt_internal_read_repo_dependencies out_var repo_dir)
    set(seen ${ARGN})
    set(dependencies "")
    set(in_dependencies_section FALSE)
    set(dependencies_file "${repo_dir}/dependencies.yaml")
    if(EXISTS "${dependencies_file}")
        file(STRINGS "${dependencies_file}" lines)
        foreach(line IN LISTS lines)
            if(line MATCHES "^([^ ]+):")
                if(CMAKE_MATCH_1 STREQUAL "dependencies")
                    set(in_dependencies_section TRUE)
                else()
                    set(in_dependencies_section FALSE)
                endif()
            elseif(in_dependencies_section AND line MATCHES "^  (.+):$")
                set(dependency "${CMAKE_MATCH_1}")
                set(dependency_repo_dir "${repo_dir}/${dependency}")
                string(REGEX MATCH "[^/]+$" dependency "${dependency}")
                if(NOT dependency IN_LIST seen)
                    qt_internal_read_repo_dependencies(subdeps "${dependency_repo_dir}"
                        ${seen} ${dependency})
                    list(APPEND dependencies ${subdeps} ${dependency})
                endif()
            endif()
        endforeach()
        list(REMOVE_DUPLICATES dependencies)
    endif()
    set(${out_var} "${dependencies}" PARENT_SCOPE)
endfunction()

set(QT_BACKUP_CMAKE_INSTALL_PREFIX_BEFORE_EXTRA_INCLUDE "${CMAKE_INSTALL_PREFIX}")

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/QtBuildInternalsExtra.cmake")
    include(${CMAKE_CURRENT_LIST_DIR}/QtBuildInternalsExtra.cmake)
endif()

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

macro(qt_set_up_build_internals_paths)
    # Set up the paths for the cmake modules located in the prefix dir. Prepend, so the paths are
    # least important compared to the source dir ones, but more important than command line
    # provided ones.
    set(QT_CMAKE_MODULE_PATH "${QT_BUILD_INTERNALS_PATH}/../${QT_CMAKE_EXPORT_NAMESPACE}")
    list(PREPEND CMAKE_MODULE_PATH "${QT_CMAKE_MODULE_PATH}")

    # Prepend the qtbase source cmake directory to CMAKE_MODULE_PATH,
    # so that if a change is done in cmake/QtBuild.cmake, it gets automatically picked up when
    # building qtdeclarative, rather than having to build qtbase first (which will copy
    # QtBuild.cmake to the build dir). This is similar to qmake non-prefix builds, where the
    # source qtbase/mkspecs directory is used.
    # TODO: Clean this up, together with qt_internal_try_compile_binary_for_strip to only use the
    # the qtbase sources when building qtbase. And perhaps also when doing a non-prefix
    # developer-build.
    if(EXISTS "${QT_SOURCE_TREE}/cmake")
        list(PREPEND CMAKE_MODULE_PATH "${QT_SOURCE_TREE}/cmake")
    endif()

    # If the repo has its own cmake modules, include those in the module path.
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
        list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
    endif()

    # Find the cmake files when doing a standalone tests build.
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
        list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
    endif()
endmacro()

# Set up the build internal paths unless explicitly requested not to.
if(NOT QT_BUILD_INTERNALS_SKIP_CMAKE_MODULE_PATH_ADDITION)
    qt_set_up_build_internals_paths()
endif()

# Define some constants to check for certain platforms, etc.
# Needs to be loaded before qt_repo_build() to handle require() clauses before even starting a repo
# build.
include(QtPlatformSupport)

function(qt_build_internals_disable_pkg_config_if_needed)
    # pkg-config should not be used by default on Darwin and Windows platforms (and QNX), as defined
    # in the qtbase/configure.json. Unfortunately by the time the feature is evaluated there are
    # already a few find_package() calls that try to use the FindPkgConfig module.
    # Thus, we have to duplicate the condition logic here and disable pkg-config for those platforms
    # by default.
    # We also need to check if the pkg-config executable exists, to mirror the condition test in
    # configure.json. We do that by trying to find the executable ourselves, and not delegating to
    # the FindPkgConfig module because that has more unwanted side-effects.
    #
    # Note that on macOS, if the pkg-config feature is enabled by the user explicitly, we will also
    # tell CMake to consider paths like /usr/local (Homebrew) as system paths when looking for
    # packages.
    # We have to do that because disabling these paths but keeping pkg-config
    # enabled won't enable finding all system libraries via pkg-config alone, many libraries can
    # only be found via FooConfig.cmake files which means /usr/local should be in the system prefix
    # path.

    set(pkg_config_enabled ON)
    qt_build_internals_find_pkg_config_executable()

    if(APPLE OR WIN32 OR QNX OR ANDROID OR WASM OR (NOT PKG_CONFIG_EXECUTABLE))
        set(pkg_config_enabled OFF)
    endif()

    # Features won't have been evaluated yet if this is the first run, have to evaluate this here
    if ((NOT DEFINED "FEATURE_pkg_config") AND (DEFINED "INPUT_pkg_config")
            AND (NOT "${INPUT_pkg_config}" STREQUAL "undefined")
            AND (NOT "${INPUT_pkg_config}" STREQUAL ""))
        if(INPUT_pkg_config)
            set(FEATURE_pkg_config ON)
        else()
            set(FEATURE_pkg_config OFF)
        endif()
    endif()

    # If user explicitly specified a value for the feature, honor it, even if it might break
    # the build.
    if(DEFINED FEATURE_pkg_config)
        if(FEATURE_pkg_config)
            set(pkg_config_enabled ON)
        else()
            set(pkg_config_enabled OFF)
        endif()
    endif()

    set(FEATURE_pkg_config "${pkg_config_enabled}" CACHE STRING "Using pkg-config")
    if(NOT pkg_config_enabled)
        qt_build_internals_disable_pkg_config()
    else()
        unset(PKG_CONFIG_EXECUTABLE CACHE)
    endif()
endfunction()

# This is a copy of the first few lines in FindPkgConfig.cmake.
function(qt_build_internals_find_pkg_config_executable)
    # find pkg-config, use PKG_CONFIG if set
    if((NOT PKG_CONFIG_EXECUTABLE) AND (NOT "$ENV{PKG_CONFIG}" STREQUAL ""))
      set(PKG_CONFIG_EXECUTABLE "$ENV{PKG_CONFIG}" CACHE FILEPATH "pkg-config executable")
    endif()
    find_program(PKG_CONFIG_EXECUTABLE NAMES pkg-config DOC "pkg-config executable")
    mark_as_advanced(PKG_CONFIG_EXECUTABLE)
endfunction()

function(qt_build_internals_disable_pkg_config)
    # Disable pkg-config by setting an empty executable path. There's no documented way to
    # mark the package as not found, but we can force all pkg_check_modules calls to do nothing
    # by setting the variable to an empty value.
    set(PKG_CONFIG_EXECUTABLE "" CACHE STRING "Disabled pkg-config usage." FORCE)
endfunction()

if(NOT QT_BUILD_INTERNALS_SKIP_PKG_CONFIG_ADJUSTMENT)
    qt_build_internals_disable_pkg_config_if_needed()
endif()

macro(qt_build_internals_find_pkg_config)
    # Find package config once before any system prefix modifications.
    find_package(PkgConfig QUIET)
endmacro()

if(NOT QT_BUILD_INTERNALS_SKIP_FIND_PKG_CONFIG)
    qt_build_internals_find_pkg_config()
endif()

function(qt_build_internals_set_up_system_prefixes)
    if(APPLE AND NOT FEATURE_pkg_config)
        # Remove /usr/local and other paths like that which CMake considers as system prefixes on
        # darwin platforms. CMake considers them as system prefixes, but in qmake / Qt land we only
        # consider the SDK path as a system prefix.
        # 3rd party libraries in these locations should not be picked up when building Qt,
        # unless opted-in via the pkg-config feature, which in turn will disable this behavior.
        #
        # Note that we can't remove /usr as a system prefix path, because many programs won't be
        # found then (e.g. perl).
        set(QT_CMAKE_SYSTEM_PREFIX_PATH_BACKUP "${CMAKE_SYSTEM_PREFIX_PATH}" PARENT_SCOPE)
        set(QT_CMAKE_SYSTEM_FRAMEWORK_PATH_BACKUP "${CMAKE_SYSTEM_FRAMEWORK_PATH}" PARENT_SCOPE)

        list(REMOVE_ITEM CMAKE_SYSTEM_PREFIX_PATH
            "/usr/local" # Homebrew
            "/opt/homebrew" # Apple Silicon Homebrew
            "/usr/X11R6"
            "/usr/pkg"
            "/opt"
            "/sw" # Fink
            "/opt/local" # MacPorts
        )
        if(_CMAKE_INSTALL_DIR)
            list(REMOVE_ITEM CMAKE_SYSTEM_PREFIX_PATH "${_CMAKE_INSTALL_DIR}")
        endif()
        list(REMOVE_ITEM CMAKE_SYSTEM_FRAMEWORK_PATH "~/Library/Frameworks")
        set(CMAKE_SYSTEM_PREFIX_PATH "${CMAKE_SYSTEM_PREFIX_PATH}" PARENT_SCOPE)
        set(CMAKE_SYSTEM_FRAMEWORK_PATH "${CMAKE_SYSTEM_FRAMEWORK_PATH}" PARENT_SCOPE)

        # Also tell qt_find_package() not to use PATH when looking for packages.
        # We can't simply set CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH to OFF because that will break
        # find_program(), and for instance ccache won't be found.
        # That's why we set a different variable which is used by qt_find_package.
        set(QT_NO_USE_FIND_PACKAGE_SYSTEM_ENVIRONMENT_PATH "ON" PARENT_SCOPE)
    endif()
endfunction()

if(NOT QT_BUILD_INTERNALS_SKIP_SYSTEM_PREFIX_ADJUSTMENT)
    qt_build_internals_set_up_system_prefixes()
endif()

# The macro sets all the necessary pre-conditions and setup consistent environment for building
# the Qt repository. It has to be called right after the find_package(Qt6 COMPONENTS BuildInternals)
# call. Otherwise we cannot make sure that all the required policies will be applied to the Qt
# components that are involved in build procedure.
macro(qt_internal_project_setup)
    # Check for the minimum CMake version.
    include(QtCMakeVersionHelpers)
    qt_internal_require_suitable_cmake_version()
    qt_internal_upgrade_cmake_policies()
endmacro()

macro(qt_build_internals_set_up_private_api)
    # TODO: this call needs to be removed once all repositories got the qtbase update
    qt_internal_project_setup()

    # Qt specific setup common for all modules:
    include(QtSetup)
    include(FeatureSummary)

    # Optionally include a repo specific Setup module.
    include(${PROJECT_NAME}Setup OPTIONAL)
    include(QtRepoSetup OPTIONAL)

    # Find Apple frameworks if needed.
    qt_find_apple_system_frameworks()

    # Decide whether tools will be built.
    qt_check_if_tools_will_be_built()
endmacro()

# find all targets defined in $subdir by recursing through all added subdirectories
# populates $qt_repo_targets with a ;-list of non-UTILITY targets
macro(qt_build_internals_get_repo_targets subdir)
    get_directory_property(_targets DIRECTORY "${subdir}" BUILDSYSTEM_TARGETS)
    if(_targets)
        foreach(_target IN LISTS _targets)
            get_target_property(_type ${_target} TYPE)
            if(NOT ${_type} STREQUAL "UTILITY")
                list(APPEND qt_repo_targets "${_target}")
            endif()
        endforeach()
    endif()

    get_directory_property(_directories DIRECTORY "${subdir}" SUBDIRECTORIES)
    if (_directories)
        foreach(_directory IN LISTS _directories)
            qt_build_internals_get_repo_targets("${_directory}")
        endforeach()
    endif()
endmacro()

# add toplevel targets for each subdirectory, e.g. qtbase_src
function(qt_build_internals_add_toplevel_targets)
    set(qt_repo_target_all "")
    get_directory_property(directories DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" SUBDIRECTORIES)
    foreach(directory IN LISTS directories)
        set(qt_repo_targets "")
        get_filename_component(qt_repo_target_basename ${directory} NAME)
        qt_build_internals_get_repo_targets("${directory}")
        if (qt_repo_targets)
            set(qt_repo_target_name "${qt_repo_targets_name}_${qt_repo_target_basename}")
            message(DEBUG "${qt_repo_target_name} depends on ${qt_repo_targets}")
            add_custom_target("${qt_repo_target_name}"
                COMMENT "Building everything in ${qt_repo_targets_name}/${qt_repo_target_basename}")
            add_dependencies("${qt_repo_target_name}" ${qt_repo_targets})
            list(APPEND qt_repo_target_all "${qt_repo_target_name}")
        endif()
    endforeach()
    if (qt_repo_target_all)
        # Note qt_repo_targets_name is different from qt_repo_target_name that is used above.
        add_custom_target("${qt_repo_targets_name}"
                            COMMENT "Building everything in ${qt_repo_targets_name}")
        add_dependencies("${qt_repo_targets_name}" ${qt_repo_target_all})
        message(DEBUG "${qt_repo_targets_name} depends on ${qt_repo_target_all}")
    endif()
endfunction()

macro(qt_enable_cmake_languages)
    set(__qt_required_language_list C CXX)
    set(__qt_platform_required_language_list )

    if(APPLE)
        list(APPEND __qt_platform_required_language_list OBJC OBJCXX)
    endif()

    foreach(__qt_lang ${__qt_required_language_list})
        enable_language(${__qt_lang})
    endforeach()

    foreach(__qt_lang ${__qt_platform_required_language_list})
        enable_language(${__qt_lang})
    endforeach()

    # The qtbase call is handled in qtbase/CMakeLists.txt.
    # This call is used for projects other than qtbase, including for other project's standalone
    # tests.
    # Because the function uses QT_FEATURE_foo values, it's important that find_package(Qt6Core) is
    # called before this function. but that's usually the case for Qt repos.
    if(NOT PROJECT_NAME STREQUAL "QtBase")
        qt_internal_set_up_config_optimizations_like_in_qmake()
    endif()
endmacro()

# Minimum setup required to have any CMakeList.txt build as as a standalone
# project after importing BuildInternals
macro(qt_prepare_standalone_project)
    qt_set_up_build_internals_paths()
    qt_build_internals_set_up_private_api()
    qt_enable_cmake_languages()
endmacro()

# Define a repo target set, and store accompanying information.
#
# A repo target set is a subset of targets in a Qt module repository. To build a repo target set,
# set QT_BUILD_SINGLE_REPO_TARGET_SET to the name of the repo target set.
#
# This function is to be called in the top-level project file of a repository,
# before qt_internal_prepare_single_repo_target_set_build()
#
# This function stores information in variables of the parent scope.
#
# Positional Arguments:
#   name - The name of this repo target set.
#
# Named Arguments:
#   DEPENDS - List of Qt6 COMPONENTS that are build dependencies of this repo target set.
function(qt_internal_define_repo_target_set name)
    set(oneValueArgs DEPENDS)
    set(prefix QT_REPO_TARGET_SET_)
    cmake_parse_arguments(${prefix}${name} "" ${oneValueArgs} "" ${ARGN})
    foreach(arg IN LISTS oneValueArgs)
        set(${prefix}${name}_${arg} ${${prefix}${name}_${arg}} PARENT_SCOPE)
    endforeach()
    set(QT_REPO_KNOWN_TARGET_SETS "${QT_REPO_KNOWN_TARGET_SETS};${name}" PARENT_SCOPE)
endfunction()

# Setup a single repo target set build if QT_BUILD_SINGLE_REPO_TARGET_SET is defined.
#
# This macro must be called in the top-level project file of the repository after all repo target
# sets have been defined.
macro(qt_internal_prepare_single_repo_target_set_build)
    if(DEFINED QT_BUILD_SINGLE_REPO_TARGET_SET)
        if(NOT QT_BUILD_SINGLE_REPO_TARGET_SET IN_LIST QT_REPO_KNOWN_TARGET_SETS)
            message(FATAL_ERROR
                "Repo target set '${QT_BUILD_SINGLE_REPO_TARGET_SET}' is undefined.")
        endif()
        message(STATUS
            "Preparing single repo target set build of ${QT_BUILD_SINGLE_REPO_TARGET_SET}")
        if (NOT "${QT_REPO_TARGET_SET_${QT_BUILD_SINGLE_REPO_TARGET_SET}_DEPENDS}" STREQUAL "")
            find_package(${INSTALL_CMAKE_NAMESPACE} ${PROJECT_VERSION} CONFIG REQUIRED
                COMPONENTS ${QT_REPO_TARGET_SET_${QT_BUILD_SINGLE_REPO_TARGET_SET}_DEPENDS})
        endif()
    endif()
endmacro()

macro(qt_build_repo_begin)
    list(APPEND CMAKE_MESSAGE_CONTEXT "${PROJECT_NAME}")

    qt_build_internals_set_up_private_api()

    # Prevent installation in non-prefix builds.
    # We need to associate targets with export names, and that is only possible to do with the
    # install(TARGETS) command. But in a non-prefix build, we don't want to install anything.
    # To make sure that developers don't accidentally run make install, add bail out code to
    # cmake_install.cmake.
    if(NOT QT_WILL_INSTALL)
        # In a top-level build, print a message only in qtbase, which is the first repository.
        if(NOT QT_SUPERBUILD OR (PROJECT_NAME STREQUAL "QtBase"))
            install(CODE [[message(FATAL_ERROR
                    "Qt was configured as non-prefix build. "
                    "Installation is not supported for this arrangement.")]])
        endif()

        install(CODE [[return()]])
    endif()

    qt_enable_cmake_languages()

    qt_internal_generate_binary_strip_wrapper()

    # Add global docs targets that will work both for per-repo builds, and super builds.
    if(NOT TARGET docs)
        add_custom_target(docs)
        add_custom_target(prepare_docs)
        add_custom_target(generate_docs)
        add_custom_target(html_docs)
        add_custom_target(qch_docs)
        add_custom_target(install_html_docs)
        add_custom_target(install_qch_docs)
        add_custom_target(install_docs)
        add_dependencies(html_docs generate_docs)
        add_dependencies(docs html_docs qch_docs)
        add_dependencies(install_docs install_html_docs install_qch_docs)
    endif()

    if(NOT TARGET sync_headers)
        add_custom_target(sync_headers)
    endif()

    # The special target that we use to sync 3rd-party headers before the gn run when building
    # qtwebengine in top-level builds.
    if(NOT TARGET thirdparty_sync_headers)
        add_custom_target(thirdparty_sync_headers)
    endif()

    # Add global qt_plugins, qpa_plugins and qpa_default_plugins convenience custom targets.
    # Internal executables will add a dependency on the qpa_default_plugins target,
    # so that building and running a test ensures it won't fail at runtime due to a missing qpa
    # plugin.
    if(NOT TARGET qt_plugins)
        add_custom_target(qt_plugins)
        add_custom_target(qpa_plugins)
        add_custom_target(qpa_default_plugins)
    endif()

    string(TOLOWER ${PROJECT_NAME} project_name_lower)

    # Target to build all plugins that are part of the current repo.
    set(qt_repo_plugins "qt_plugins_${project_name_lower}")
    if(NOT TARGET ${qt_repo_plugins})
        add_custom_target(${qt_repo_plugins})
    endif()

    # Target to build all plugins that are part of the current repo and the current repo's
    # dependencies plugins. Used for external project example dependencies.
    set(qt_repo_plugins_recursive "${qt_repo_plugins}_recursive")
    if(NOT TARGET ${qt_repo_plugins_recursive})
        add_custom_target(${qt_repo_plugins_recursive})
        add_dependencies(${qt_repo_plugins_recursive} "${qt_repo_plugins}")
    endif()

    qt_internal_read_repo_dependencies(qt_repo_deps "${PROJECT_SOURCE_DIR}")
    if(qt_repo_deps)
        foreach(qt_repo_dep IN LISTS qt_repo_deps)
            if(TARGET qt_plugins_${qt_repo_dep})
                message(DEBUG
                    "${qt_repo_plugins_recursive} depends on qt_plugins_${qt_repo_dep}")
                add_dependencies(${qt_repo_plugins_recursive} "qt_plugins_${qt_repo_dep}")
            endif()
        endforeach()
    endif()

    set(qt_repo_targets_name ${project_name_lower})
    set(qt_docs_target_name docs_${project_name_lower})
    set(qt_docs_prepare_target_name prepare_docs_${project_name_lower})
    set(qt_docs_generate_target_name generate_docs_${project_name_lower})
    set(qt_docs_html_target_name html_docs_${project_name_lower})
    set(qt_docs_qch_target_name qch_docs_${project_name_lower})
    set(qt_docs_install_html_target_name install_html_docs_${project_name_lower})
    set(qt_docs_install_qch_target_name install_qch_docs_${project_name_lower})
    set(qt_docs_install_target_name install_docs_${project_name_lower})

    add_custom_target(${qt_docs_target_name})
    add_custom_target(${qt_docs_prepare_target_name})
    add_custom_target(${qt_docs_generate_target_name})
    add_custom_target(${qt_docs_qch_target_name})
    add_custom_target(${qt_docs_html_target_name})
    add_custom_target(${qt_docs_install_html_target_name})
    add_custom_target(${qt_docs_install_qch_target_name})
    add_custom_target(${qt_docs_install_target_name})

    add_dependencies(${qt_docs_generate_target_name} ${qt_docs_prepare_target_name})
    add_dependencies(${qt_docs_html_target_name} ${qt_docs_generate_target_name})
    add_dependencies(${qt_docs_target_name} ${qt_docs_html_target_name} ${qt_docs_qch_target_name})
    add_dependencies(${qt_docs_install_target_name} ${qt_docs_install_html_target_name} ${qt_docs_install_qch_target_name})

    # Make top-level prepare_docs target depend on the repository-level prepare_docs_<repo> target.
    add_dependencies(prepare_docs ${qt_docs_prepare_target_name})

    # Make top-level install_*_docs targets depend on the repository-level install_*_docs targets.
    add_dependencies(install_html_docs ${qt_docs_install_html_target_name})
    add_dependencies(install_qch_docs ${qt_docs_install_qch_target_name})

    # Add host_tools meta target, so that developrs can easily build only tools and their
    # dependencies when working in qtbase.
    if(NOT TARGET host_tools)
        add_custom_target(host_tools)
        add_custom_target(bootstrap_tools)
    endif()

    # Add benchmark meta target. It's collection of all benchmarks added/registered by
    # 'qt_internal_add_benchmark' helper.
    if(NOT TARGET benchmark)
        add_custom_target(benchmark)
    endif()

    if(QT_INTERNAL_SYNCED_MODULES)
        set_property(GLOBAL PROPERTY _qt_synced_modules ${QT_INTERNAL_SYNCED_MODULES})
    endif()
endmacro()

macro(qt_build_repo_end)
    if(NOT QT_BUILD_STANDALONE_TESTS)
        # Delayed actions on some of the Qt targets:
        include(QtPostProcess)

        # Install the repo-specific cmake find modules.
        qt_path_join(__qt_repo_install_dir ${QT_CONFIG_INSTALL_DIR} ${INSTALL_CMAKE_NAMESPACE})
        qt_path_join(__qt_repo_build_dir ${QT_CONFIG_BUILD_DIR} ${INSTALL_CMAKE_NAMESPACE})

        if(NOT PROJECT_NAME STREQUAL "QtBase")
            if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
                qt_copy_or_install(DIRECTORY cmake/
                    DESTINATION "${__qt_repo_install_dir}"
                    FILES_MATCHING PATTERN "Find*.cmake"
                )
                if(QT_SUPERBUILD AND QT_WILL_INSTALL)
                    file(COPY cmake/
                         DESTINATION "${__qt_repo_build_dir}"
                         FILES_MATCHING PATTERN "Find*.cmake"
                    )
                endif()
            endif()
        endif()

        if(NOT QT_SUPERBUILD)
            qt_print_feature_summary()
        endif()
    endif()

    qt_build_internals_add_toplevel_targets()

    if(NOT QT_SUPERBUILD)
        qt_print_build_instructions()
    endif()

    get_property(synced_modules GLOBAL PROPERTY _qt_synced_modules)
    if(synced_modules)
        set(QT_INTERNAL_SYNCED_MODULES ${synced_modules} CACHE INTERNAL
            "List of the synced modules. Prevents running syncqt.cpp after the first configuring.")
    endif()

    if(NOT QT_SUPERBUILD)
        qt_internal_save_previously_visited_packages()
    endif()

    if(QT_INTERNAL_FRESH_REQUESTED)
        set(QT_INTERNAL_FRESH_REQUESTED "FALSE" CACHE INTERNAL "")
    endif()

    list(POP_BACK CMAKE_MESSAGE_CONTEXT)
endmacro()

macro(qt_build_repo)
    qt_build_repo_begin(${ARGN})

    qt_build_repo_impl_find_package_tests()
    qt_build_repo_impl_src()
    qt_build_repo_impl_tools()
    qt_build_repo_impl_tests()

    qt_build_repo_end()

    qt_build_repo_impl_examples()
endmacro()

macro(qt_build_repo_impl_find_package_tests)
    # If testing is enabled, try to find the qtbase Test package.
    # Do this before adding src, because there might be test related conditions
    # in source.
    if (QT_BUILD_TESTS AND NOT QT_BUILD_STANDALONE_TESTS)
        # When looking for the Test package, do it using the Qt6 package version, in case if
        # PROJECT_VERSION is following a different versioning scheme.
        if(Qt6_VERSION)
            set(_qt_build_repo_impl_find_package_tests_version "${Qt6_VERSION}")
        else()
            set(_qt_build_repo_impl_find_package_tests_version "${PROJECT_VERSION}")
        endif()

        find_package(Qt6
            "${_qt_build_repo_impl_find_package_tests_version}"
            CONFIG REQUIRED COMPONENTS Test)
        unset(_qt_build_repo_impl_find_package_tests_version)
    endif()
endmacro()

macro(qt_build_repo_impl_src)
    if(NOT QT_BUILD_STANDALONE_TESTS)
        if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/CMakeLists.txt")
            add_subdirectory(src)
        endif()
    endif()
    if(QT_FEATURE_lttng AND NOT TARGET LTTng::UST)
        qt_find_package(LTTngUST PROVIDED_TARGETS LTTng::UST
                        MODULE_NAME global QMAKE_LIB lttng-ust)
    endif()
endmacro()

macro(qt_build_repo_impl_tools)
    if(NOT QT_BUILD_STANDALONE_TESTS)
        if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tools/CMakeLists.txt")
            add_subdirectory(tools)
        endif()
    endif()
endmacro()

macro(qt_build_repo_impl_tests)
    if (QT_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
        add_subdirectory(tests)
        if(NOT QT_BUILD_TESTS_BY_DEFAULT)
            set_property(DIRECTORY tests PROPERTY EXCLUDE_FROM_ALL TRUE)
        endif()
    endif()
endmacro()

macro(qt_build_repo_impl_examples)
    if(QT_BUILD_EXAMPLES
            AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt"
            AND NOT QT_BUILD_STANDALONE_TESTS)
        message(STATUS "Configuring examples.")
        add_subdirectory(examples)
    endif()
endmacro()

macro(qt_set_up_standalone_tests_build)
    # Remove this macro once all usages of it have been removed.
    # Standalone tests are not handled via the main repo project and qt_build_tests.
endmacro()

function(qt_get_standalone_tests_config_files_path out_var)
    set(path "${QT_CONFIG_INSTALL_DIR}/${INSTALL_CMAKE_NAMESPACE}BuildInternals/StandaloneTests")

    # QT_CONFIG_INSTALL_DIR is relative in prefix builds.
    if(QT_WILL_INSTALL)
        if(DEFINED CMAKE_STAGING_PREFIX)
            qt_path_join(path "${CMAKE_STAGING_PREFIX}" "${path}")
        else()
            qt_path_join(path "${CMAKE_INSTALL_PREFIX}" "${path}")
        endif()
    endif()

    set("${out_var}" "${path}" PARENT_SCOPE)
endfunction()

function(qt_internal_get_standalone_tests_config_file_name out_var)
    # When doing a "single repo target set" build (like in qtscxqml) ensure we use a unique tests
    # config file for each repo target set. Using the PROJECT_NAME only is not enough because
    # the same file will be overridden with different content on each repo set install.
    set(tests_config_file_name "${PROJECT_NAME}")

    if(QT_BUILD_SINGLE_REPO_TARGET_SET)
        string(APPEND tests_config_file_name "RepoSet${QT_BUILD_SINGLE_REPO_TARGET_SET}")
    endif()
    string(APPEND tests_config_file_name "TestsConfig.cmake")

    set(${out_var} "${tests_config_file_name}" PARENT_SCOPE)
endfunction()

macro(qt_build_tests)
    set(CMAKE_UNITY_BUILD OFF)

    if(QT_BUILD_STANDALONE_TESTS)
        # Find location of TestsConfig.cmake. These contain the modules that need to be
        # find_package'd when testing.
        qt_get_standalone_tests_config_files_path(_qt_build_tests_install_prefix)

        qt_internal_get_standalone_tests_config_file_name(_qt_tests_config_file_name)
        set(_qt_standalone_tests_config_file_path
            "${_qt_build_tests_install_prefix}/${_qt_tests_config_file_name}")
        include("${_qt_standalone_tests_config_file_path}"
            OPTIONAL
            RESULT_VARIABLE _qt_standalone_tests_included)
        if(NOT _qt_standalone_tests_included)
            message(DEBUG
                "Standalone tests config file not included because it does not exist: "
                "${_qt_standalone_tests_config_file_path}"
            )
        else()
            message(DEBUG
                "Standalone tests config file included successfully: "
                "${_qt_standalone_tests_config_file_path}"
            )
        endif()

        unset(_qt_standalone_tests_config_file_path)
        unset(_qt_standalone_tests_included)
        unset(_qt_tests_config_file_name)

        # Of course we always need the test module as well.
        # When looking for the Test package, do it using the Qt6 package version, in case if
        # PROJECT_VERSION is following a different versioning scheme.
        if(Qt6_VERSION)
            set(_qt_build_tests_package_version "${Qt6_VERSION}")
        else()
            set(_qt_build_tests_package_version "${PROJECT_VERSION}")
        endif()
        find_package(Qt6 "${_qt_build_tests_package_version}" CONFIG REQUIRED COMPONENTS Test)
        unset(_qt_build_tests_package_version)

        # Set language standards after finding Core, because that's when the relevant
        # feature variables are available, and the call in QtSetup is too early when building
        # standalone tests, because Core was not find_package()'d yet.
        qt_set_language_standards()

        if(NOT QT_SUPERBUILD)
            # Set up fake standalone tests install prefix, so we don't pollute the Qt install
            # prefix. For super builds it needs to be done in qt5/CMakeLists.txt.
            qt_set_up_fake_standalone_tests_install_prefix()
        endif()
    else()
        if(ANDROID)
            # When building in-tree tests we need to specify the QT_ANDROID_ABIS list. Since we
            # build Qt for the single ABI, build tests for this ABI only.
            set(QT_ANDROID_ABIS "${CMAKE_ANDROID_ARCH_ABI}")
        endif()
    endif()

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/auto/CMakeLists.txt")
        add_subdirectory(auto)
    endif()
    if(NOT QT_BUILD_MINIMAL_STATIC_TESTS AND NOT QT_BUILD_MINIMAL_ANDROID_MULTI_ABI_TESTS)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/baseline/CMakeLists.txt")
            add_subdirectory(baseline)
        endif()
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/CMakeLists.txt" AND QT_BUILD_BENCHMARKS)
            add_subdirectory(benchmarks)
        endif()
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/manual/CMakeLists.txt" AND QT_BUILD_MANUAL_TESTS)
            add_subdirectory(manual)
        endif()
    endif()

    set(CMAKE_UNITY_BUILD ${QT_UNITY_BUILD})
endmacro()

function(qt_compute_relative_path_from_cmake_config_dir_to_prefix)
    # Compute the reverse relative path from the CMake config dir to the install prefix.
    # This is used in QtBuildInternalsExtras to create a relocatable relative install prefix path.
    # This path is used for finding syncqt and other things, regardless of initial install prefix
    # (e.g installed Qt was archived and unpacked to a different path on a different machine).
    #
    # This is meant to be called only once when configuring qtbase.
    #
    # Similar code exists in Qt6CoreConfigExtras.cmake.in and src/corelib/CMakeLists.txt which
    # might not be needed anymore.
    if(CMAKE_STAGING_PREFIX)
        set(__qt_prefix "${CMAKE_STAGING_PREFIX}")
    else()
        set(__qt_prefix "${CMAKE_INSTALL_PREFIX}")
    endif()

    if(QT_WILL_INSTALL)
        get_filename_component(clean_config_prefix
                               "${__qt_prefix}/${QT_CONFIG_INSTALL_DIR}" ABSOLUTE)
    else()
        get_filename_component(clean_config_prefix "${QT_CONFIG_BUILD_DIR}" ABSOLUTE)
    endif()
    file(RELATIVE_PATH
         qt_path_from_cmake_config_dir_to_prefix
         "${clean_config_prefix}" "${__qt_prefix}")
     set(qt_path_from_cmake_config_dir_to_prefix "${qt_path_from_cmake_config_dir_to_prefix}"
         PARENT_SCOPE)
endfunction()

function(qt_get_relocatable_install_prefix out_var)
    # We need to compute it only once while building qtbase. Afterwards it's loaded from
    # QtBuildInternalsExtras.cmake.
    if(QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX)
        return()
    endif()
    # The QtBuildInternalsExtras value is dynamically computed, whereas the initial qtbase
    # configuration uses an absolute path.
    set(${out_var} "${CMAKE_INSTALL_PREFIX}" PARENT_SCOPE)
endfunction()

function(qt_set_up_fake_standalone_tests_install_prefix)
    # Set a fake local (non-cache) CMAKE_INSTALL_PREFIX.
    # Needed for standalone tests, we don't want to accidentally install a test into the Qt prefix.
    # Allow opt-out, if a user knows what they're doing.
    if(QT_NO_FAKE_STANDALONE_TESTS_INSTALL_PREFIX)
        return()
    endif()
    set(new_install_prefix "${CMAKE_BINARY_DIR}/fake_prefix")

    # It's IMPORTANT that this is not a cache variable. Otherwise
    # qt_get_standalone_tests_confg_files_path() will not work on re-configuration.
    message(STATUS
            "Setting local standalone test install prefix (non-cached) to '${new_install_prefix}'.")
    set(CMAKE_INSTALL_PREFIX "${new_install_prefix}" PARENT_SCOPE)

    # We also need to clear the staging prefix if it's set, otherwise CMake will modify any computed
    # rpaths containing the staging prefix to point to the new fake prefix, which is not what we
    # want. This replacement is done in cmComputeLinkInformation::GetRPath().
    #
    # By clearing the staging prefix for the standalone tests, any detected link time
    # rpaths will be embedded as-is, which will point to the place where Qt was installed (aka
    # the staging prefix).
    if(DEFINED CMAKE_STAGING_PREFIX)
        message(STATUS "Clearing local standalone test staging prefix (non-cached).")
        set(CMAKE_STAGING_PREFIX "" PARENT_SCOPE)
    endif()
endfunction()

# Mean to be called when configuring examples as part of the main build tree, as well as for CMake
# tests (tests that call CMake to try and build CMake applications).
macro(qt_internal_set_up_build_dir_package_paths)
    list(PREPEND CMAKE_PREFIX_PATH "${QT_BUILD_DIR}/${INSTALL_LIBDIR}/cmake")
    # Make sure the CMake config files do not recreate the already-existing targets
    set(QT_NO_CREATE_TARGETS TRUE)
endmacro()

macro(qt_examples_build_begin)
    set(options EXTERNAL_BUILD)
    set(singleOpts "")
    set(multiOpts DEPENDS)

    cmake_parse_arguments(arg "${options}" "${singleOpts}" "${multiOpts}" ${ARGN})

    set(CMAKE_UNITY_BUILD OFF)

    # Use by qt_internal_add_example.
    set(QT_EXAMPLE_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

    if(arg_EXTERNAL_BUILD AND QT_BUILD_EXAMPLES_AS_EXTERNAL)
        # Examples will be built using ExternalProject.
        # We depend on all plugins built as part of the current repo as well as current repo's
        # dependencies plugins, to prevent opportunities for
        # weird errors associated with loading out-of-date plugins from
        # unrelated Qt modules.
        # We also depend on all targets from this repo's src and tools subdirectories
        # to ensure that we've built anything that a find_package() call within
        # an example might use. Projects can add further dependencies if needed,
        # but that should rarely be necessary.
        set(QT_EXAMPLE_DEPENDENCIES ${qt_repo_plugins_recursive} ${arg_DEPENDS})

        if(TARGET ${qt_repo_targets_name}_src)
            list(APPEND QT_EXAMPLE_DEPENDENCIES ${qt_repo_targets_name}_src)
        endif()

        if(TARGET ${qt_repo_targets_name}_tools)
            list(APPEND QT_EXAMPLE_DEPENDENCIES ${qt_repo_targets_name}_tools)
        endif()

        set(QT_IS_EXTERNAL_EXAMPLES_BUILD TRUE)

        string(TOLOWER ${PROJECT_NAME} project_name_lower)
        if(NOT TARGET examples)
            if(QT_BUILD_EXAMPLES_BY_DEFAULT)
                add_custom_target(examples ALL)
            else()
                add_custom_target(examples)
            endif()
        endif()
        if(NOT TARGET examples_${project_name_lower})
            add_custom_target(examples_${project_name_lower})
            add_dependencies(examples examples_${project_name_lower})
        endif()

        include(ExternalProject)
    else()
        # This repo has not yet been updated to build examples in a separate
        # build from this main build, or we can't use that arrangement yet.
        # Build them directly as part of the main build instead for backward
        # compatibility.
        if(NOT BUILD_SHARED_LIBS)
            # Ordinarily, it would be an error to call return() from within a
            # macro(), but in this case we specifically want to return from the
            # caller's scope if we are doing a static build and the project
            # isn't building examples in a separate build from the main build.
            # Configuring static builds requires tools that are not available
            # until build time.
            return()
        endif()

        if(NOT QT_BUILD_EXAMPLES_BY_DEFAULT)
            set_directory_properties(PROPERTIES EXCLUDE_FROM_ALL TRUE)
        endif()
    endif()

    # TODO: Change this to TRUE when all examples in all repos are ported to use
    # qt_internal_add_example.
    # We shouldn't need to call qt_internal_set_up_build_dir_package_paths when
    # QT_IS_EXTERNAL_EXAMPLES_BUILD is TRUE.
    # Due to not all examples being ported, if we don't
    # call qt_internal_set_up_build_dir_package_paths -> set(QT_NO_CREATE_TARGETS TRUE) we'll get
    # CMake configuration errors saying we redefine Qt targets because we both build them and find
    # them as part of find_package.
    set(__qt_all_examples_ported_to_external_projects FALSE)

    # Examples that are built as part of the Qt build need to use the CMake config files from the
    # build dir, because they are not installed yet in a prefix build.
    # Prepending to CMAKE_PREFIX_PATH helps find the initial Qt6Config.cmake.
    # Prepending to QT_EXAMPLES_CMAKE_PREFIX_PATH helps find components of Qt6, because those
    # find_package calls use NO_DEFAULT_PATH, and thus CMAKE_PREFIX_PATH is ignored.
    # Prepending to CMAKE_FIND_ROOT_PATH ensures the components are found while cross-compiling
    # without setting CMAKE_FIND_ROOT_PATH_MODE_PACKAGE to BOTH.
    if(NOT QT_IS_EXTERNAL_EXAMPLES_BUILD OR NOT __qt_all_examples_ported_to_external_projects)
        qt_internal_set_up_build_dir_package_paths()
        list(PREPEND CMAKE_FIND_ROOT_PATH "${QT_BUILD_DIR}")
        list(PREPEND QT_EXAMPLES_CMAKE_PREFIX_PATH "${QT_BUILD_DIR}/${INSTALL_LIBDIR}/cmake")
    endif()

    # Because CMAKE_INSTALL_RPATH is empty by default in the repo project, examples need to have
    # it set here, so they can run when installed.
    # This means that installed examples are not relocatable at the moment. We would need to
    # annotate where each example is installed to, to be able to derive a relative rpath, and it
    # seems there's no way to query such information from CMake itself.
    set(CMAKE_INSTALL_RPATH "${_default_install_rpath}")

    install(CODE "
# Backup CMAKE_INSTALL_PREFIX because we're going to change it in each example subdirectory
# and restore it after all examples are processed so that QtFooToolsAdditionalTargetInfo.cmake
# files are installed into the original install prefix.
set(_qt_internal_examples_cmake_install_prefix_backup \"\${CMAKE_INSTALL_PREFIX}\")
")
endmacro()

macro(qt_examples_build_end)
    # We use AUTOMOC/UIC/RCC in the examples. When the examples are part of the
    # main build rather than being built in their own separate project, make
    # sure we do not fail on a fresh Qt build (e.g. the moc binary won't exist
    # yet because it is created at build time).

    # This function gets all targets below this directory (excluding custom targets and aliases)
    function(get_all_targets _result _dir)
        get_property(_subdirs DIRECTORY "${_dir}" PROPERTY SUBDIRECTORIES)
        foreach(_subdir IN LISTS _subdirs)
            get_all_targets(${_result} "${_subdir}")
        endforeach()
        get_property(_sub_targets DIRECTORY "${_dir}" PROPERTY BUILDSYSTEM_TARGETS)
        set(_real_targets "")
        if(_sub_targets)
            foreach(__target IN LISTS _sub_targets)
                get_target_property(target_type ${__target} TYPE)
                if(NOT target_type STREQUAL "UTILITY" AND NOT target_type STREQUAL "ALIAS")
                    list(APPEND _real_targets ${__target})
                endif()
            endforeach()
        endif()
        set(${_result} ${${_result}} ${_real_targets} PARENT_SCOPE)
    endfunction()

    get_all_targets(targets "${CMAKE_CURRENT_SOURCE_DIR}")

    foreach(target ${targets})
        qt_autogen_tools(${target} ENABLE_AUTOGEN_TOOLS "moc" "rcc")
        if(TARGET Qt::Widgets)
            qt_autogen_tools(${target} ENABLE_AUTOGEN_TOOLS "uic")
        endif()
        set_target_properties(${target} PROPERTIES UNITY_BUILD OFF)
    endforeach()

    install(CODE "
# Restore backed up CMAKE_INSTALL_PREFIX.
set(CMAKE_INSTALL_PREFIX \"\${_qt_internal_examples_cmake_install_prefix_backup}\")
")

    set(CMAKE_UNITY_BUILD ${QT_UNITY_BUILD})
endmacro()

function(qt_internal_add_example subdir)
    if(NOT QT_IS_EXTERNAL_EXAMPLES_BUILD)
        qt_internal_add_example_in_tree(${ARGV})
    else()
        qt_internal_add_example_external_project(${ARGV})
    endif()
endfunction()

# Use old non-ExternalProject approach, aka build in-tree with the Qt build.
function(qt_internal_add_example_in_tree subdir)
    file(RELATIVE_PATH example_rel_path
         "${QT_EXAMPLE_BASE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}")

    # Unset the default CMAKE_INSTALL_PREFIX that's generated in
    #   ${CMAKE_CURRENT_BINARY_DIR}/cmake_install.cmake
    # so we can override it with a different value in
    #   ${CMAKE_CURRENT_BINARY_DIR}/${subdir}/cmake_install.cmake
    #
    install(CODE "
# Unset the CMAKE_INSTALL_PREFIX in the current cmake_install.cmake file so that it can be
# overridden in the included add_subdirectory-specific cmake_install.cmake files instead.
unset(CMAKE_INSTALL_PREFIX)
")

    # Override the install prefix in the subdir cmake_install.cmake, so that
    # relative install(TARGETS DESTINATION) calls in example projects install where we tell them to.
    # Allow customizing the installation path of the examples. Will be used in CI.
    if(QT_INTERNAL_EXAMPLES_INSTALL_PREFIX)
        set(qt_example_install_prefix "${QT_INTERNAL_EXAMPLES_INSTALL_PREFIX}")
    else()
        set(qt_example_install_prefix "${CMAKE_INSTALL_PREFIX}/${INSTALL_EXAMPLESDIR}")
    endif()
    file(TO_CMAKE_PATH "${qt_example_install_prefix}" qt_example_install_prefix)

    set(CMAKE_INSTALL_PREFIX "${qt_example_install_prefix}/${example_rel_path}")

    # Make sure unclean example projects have their INSTALL_EXAMPLEDIR set to "."
    # Won't have any effect on example projects that don't use INSTALL_EXAMPLEDIR.
    # This plus the install prefix above takes care of installing examples where we want them to
    # be installed, while allowing us to remove INSTALL_EXAMPLEDIR code in each example
    # incrementally.
    # TODO: Remove once all repositories use qt_internal_add_example instead of add_subdirectory.
    set(QT_INTERNAL_SET_EXAMPLE_INSTALL_DIR_TO_DOT ON)

    add_subdirectory(${subdir} ${ARGN})
endfunction()

function(qt_internal_add_example_external_project subdir)
    set(options "")
    set(singleOpts NAME)
    set(multiOpts "")

    cmake_parse_arguments(PARSE_ARGV 1 arg "${options}" "${singleOpts}" "${multiOpts}")

    file(RELATIVE_PATH example_rel_path
         "${QT_EXAMPLE_BASE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}")

    if(NOT arg_NAME)
        set(arg_NAME "${subdir}")

        # qtdeclarative has calls like qt_internal_add_example(imagine/automotive)
        # so passing a nested subdirectory. Custom targets (and thus ExternalProjects) can't contain
        # slashes, so extract the last part of the path to be used as a name.
        if(arg_NAME MATCHES "/")
            string(REPLACE "/" ";" exploded_path "${arg_NAME}")
            list(POP_BACK exploded_path last_dir)
            if(NOT last_dir)
                message(FATAL_ERROR "Example subdirectory must have a name.")
            else()
                set(arg_NAME "${last_dir}")
            endif()
        endif()
    endif()

    # Likely a clash with an example subdir ExternalProject custom target of the same name.
    if(TARGET "${arg_NAME}")
        string(SHA1 rel_path_hash "${example_rel_path}")
        string(SUBSTRING "${rel_path_hash}" 0 4 short_hash)
        set(arg_NAME "${arg_NAME}-${short_hash}")
    endif()

    # TODO: Fix example builds when using Conan / install prefixes are different for each repo.
    if(QT_SUPERBUILD OR QtBase_BINARY_DIR)
        # When doing a top-level build or when building qtbase,
        # always use the Config file from the current build directory, even for prefix builds.
        # We strive to allow building examples without installing Qt first, which means we can't
        # use the install or staging Config files.
        set(qt_prefixes "${QT_BUILD_DIR}")
        set(qt_cmake_dir "${QT_CONFIG_BUILD_DIR}/${QT_CMAKE_EXPORT_NAMESPACE}")
    else()
        # This is a per-repo build that isn't the qtbase repo, so we know that
        # qtbase was found via find_package() and Qt6_DIR must be set
        set(qt_cmake_dir "${${QT_CMAKE_EXPORT_NAMESPACE}_DIR}")

        # In a prefix build of a non-qtbase repo, we want to pick up the installed Config files
        # for all repos except the one that is currently built. For the repo that is currently
        # built, we pick up the Config files from the current repo build dir instead.
        # For non-prefix builds, there's only one prefix, the main build dir.
        # Both are handled by this assignment.
        set(qt_prefixes "${QT_BUILD_DIR}")

        # Appending to QT_ADDITIONAL_PACKAGES_PREFIX_PATH helps find Qt6 components in
        # non-qtbase prefix builds because we use NO_DEFAULT_PATH in find_package calls.
        # It also handles the cross-compiling scenario where we need to adjust both the root path
        # and prefixes, with the prefixes containing lib/cmake. This leverages the infrastructure
        # previously added for Conan.
        list(APPEND QT_ADDITIONAL_PACKAGES_PREFIX_PATH ${qt_prefixes})

        # In a prefix build, look up all repo Config files in the install prefix,
        # except for the current repo, which will look in the build dir (handled above).
        if(QT_WILL_INSTALL)
            list(APPEND qt_prefixes "${QT6_INSTALL_PREFIX}")
        endif()
    endif()

    set(vars_to_pass_if_defined)
    set(var_defs)
    if(QT_HOST_PATH OR CMAKE_CROSSCOMPILING)
        list(APPEND var_defs
            -DCMAKE_TOOLCHAIN_FILE:FILEPATH=${qt_cmake_dir}/qt.toolchain.cmake
        )
    else()
        list(PREPEND CMAKE_PREFIX_PATH ${qt_prefixes})

        # Setting CMAKE_SYSTEM_NAME affects CMAKE_CROSSCOMPILING, even if it is
        # set to the same as the host, so it should only be set if it is different.
        # See https://gitlab.kitware.com/cmake/cmake/-/issues/21744
        if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND
           NOT CMAKE_SYSTEM_NAME STREQUAL CMAKE_HOST_SYSTEM_NAME)
            list(APPEND vars_to_pass_if_defined CMAKE_SYSTEM_NAME:STRING)
        endif()
    endif()

    # In multi-config mode by default we exclude building tools for configs other than the main one.
    # Trying to build an example in a non-default config using the non-installed
    # QtFooConfig.cmake files would error out saying moc is not found.
    # Make sure to build examples only with the main config.
    # When users build an example against an installed Qt they won't have this problem because
    # the generated non-main QtFooTargets-$<CONFIG>.cmake file is empty and doesn't advertise
    # a tool that is not there.
    if(QT_GENERATOR_IS_MULTI_CONFIG)
        set(CMAKE_CONFIGURATION_TYPES "${QT_MULTI_CONFIG_FIRST_CONFIG}")
    endif()

    # We need to pass the modified CXX flags of the parent project so that using sccache works
    # properly and doesn't error out due to concurrent access to the pdb files.
    # See qt_internal_set_up_config_optimizations_like_in_qmake, "/Zi" "/Z7".
    if(MSVC AND QT_FEATURE_msvc_obj_debug_info)
        qt_internal_get_enabled_languages_for_flag_manipulation(enabled_languages)
        set(configs RELWITHDEBINFO DEBUG)
        foreach(lang ${enabled_languages})
            foreach(config ${configs})
                set(flag_var_name "CMAKE_${lang}_FLAGS_${config}")
                list(APPEND vars_to_pass_if_defined "${flag_var_name}:STRING")
            endforeach()
        endforeach()
    endif()

    # When cross-compiling for a qemu target in our CI, we source an environment script
    # that sets environment variables like CC and CXX. These are parsed by CMake on initial
    # configuration to populate the cache vars CMAKE_${lang}_COMPILER.
    # If the environment variable specified not only the compiler path, but also a list of flags
    # to pass to the compiler, CMake parses those out into a separate CMAKE_${lang}_COMPILER_ARG1
    # cache variable. In such a case, we want to ensure that the external project also sees those
    # flags.
    # Unfortunately we can't do that by simply forwarding CMAKE_${lang}_COMPILER_ARG1 to the EP
    # because it breaks the compiler identification try_compile call, it simply doesn't consider
    # the cache var. From what I could gather, it's a limitation of try_compile and the list
    # of variables it considers for forwarding.
    # To fix this case, we ensure not to pass either cache variable, and let the external project
    # and its compiler identification try_compile project pick up the compiler and the flags
    # from the environment variables instead.
    foreach(lang_as_env_var CC CXX OBJC OBJCXX)
        if(lang_as_env_var STREQUAL "CC")
            set(lang_as_cache_var "C")
        else()
            set(lang_as_cache_var "${lang_as_env_var}")
        endif()
        set(lang_env_value "$ENV{${lang_as_env_var}}")
        if(lang_env_value
                AND CMAKE_${lang_as_cache_var}_COMPILER
                AND CMAKE_${lang_as_cache_var}_COMPILER_ARG1)
            # The compiler environment variable is set and specifies a list of extra flags, don't
            # forward the compiler cache vars and rely on the environment variable to be picked up
            # instead.
        else()
            list(APPEND vars_to_pass_if_defined "CMAKE_${lang_as_cache_var}_COMPILER:STRING")
        endif()
    endforeach()
    unset(lang_as_env_var)
    unset(lang_as_cache_var)
    unset(lang_env_value)

    list(APPEND vars_to_pass_if_defined
        CMAKE_BUILD_TYPE:STRING
        CMAKE_CONFIGURATION_TYPES:STRING
        CMAKE_PREFIX_PATH:STRING
        QT_EXAMPLES_CMAKE_PREFIX_PATH:STRING
        QT_ADDITIONAL_PACKAGES_PREFIX_PATH:STRING
        CMAKE_FIND_ROOT_PATH:STRING
        BUILD_SHARED_LIBS:BOOL
        CMAKE_OSX_ARCHITECTURES:STRING
        CMAKE_OSX_DEPLOYMENT_TARGET:STRING
        CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED:BOOL
        CMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH:BOOL
        CMAKE_C_COMPILER_LAUNCHER:STRING
        CMAKE_CXX_COMPILER_LAUNCHER:STRING
        CMAKE_OBJC_COMPILER_LAUNCHER:STRING
        CMAKE_OBJCXX_COMPILER_LAUNCHER:STRING
    )

    foreach(var_with_type IN LISTS vars_to_pass_if_defined)
        string(REPLACE ":" ";" key_as_list "${var_with_type}")
        list(GET key_as_list 0 var)
        if(NOT DEFINED ${var})
            continue()
        endif()

        # Preserve lists
        string(REPLACE ";" "$<SEMICOLON>" varForGenex "${${var}}")

        list(APPEND var_defs -D${var_with_type}=${varForGenex})
    endforeach()

    if(QT_INTERNAL_VERBOSE_EXAMPLES)
        list(APPEND var_defs -DCMAKE_MESSAGE_LOG_LEVEL:STRING=DEBUG)
        list(APPEND var_defs -DCMAKE_AUTOGEN_VERBOSE:BOOL=TRUE)
    endif()

    set(deps "")
    list(REMOVE_DUPLICATES QT_EXAMPLE_DEPENDENCIES)
    foreach(dep IN LISTS QT_EXAMPLE_DEPENDENCIES)
        if(TARGET ${dep})
            list(APPEND deps ${dep})
        endif()
    endforeach()

    set(independent_args)
    cmake_policy(PUSH)
    if(POLICY CMP0114)
        set(independent_args INDEPENDENT TRUE)
        cmake_policy(SET CMP0114 NEW)
    endif()

    # The USES_TERMINAL_BUILD setting forces the build step to the console pool
    # when using Ninja. This has two benefits:
    #
    #   - You see build output as it is generated instead of at the end of the
    #     build step.
    #   - Only one task can use the console pool at a time, so it effectively
    #     serializes all example build steps, thereby preventing CPU
    #     over-commitment.
    #
    # If the loss of interactivity is not so important, one can allow CPU
    # over-commitment for Ninja builds. This may result in better throughput,
    # but is not allowed by default because it can make a machine almost
    # unusable while a compilation is running.
    set(terminal_args USES_TERMINAL_BUILD TRUE)
    if(CMAKE_GENERATOR MATCHES "Ninja")
        option(QT_BUILD_EXAMPLES_WITH_CPU_OVERCOMMIT
            "Allow CPU over-commitment when building examples (Ninja only)"
        )
        if(QT_BUILD_EXAMPLES_WITH_CPU_OVERCOMMIT)
            set(terminal_args)
        endif()
    endif()

    # QT_EXAMPLE_INSTALL_MARKER
    # The goal is to install each example project into a directory that keeps the example source dir
    # hierarchy, without polluting the example projects with dirty INSTALL_EXAMPLEDIR and
    # INSTALL_EXAMPLESDIR usage.
    # E.g. ensure qtbase/examples/widgets/widgets/wiggly is installed to
    # $qt_example_install_prefix/examples/widgets/widgets/wiggly/wiggly.exe
    # $qt_example_install_prefix defaults to ${CMAKE_INSTALL_PREFIX}/${INSTALL_EXAMPLEDIR}
    # but can also be set to a custom location.
    # This needs to work both:
    #  - when using ExternalProject to build examples
    #  - when examples are built in-tree as part of Qt (no ExternalProject).
    # The reason we want to support the latter is for nicer IDE integration: a can developer can
    # work with a Qt repo and its examples using the same build dir.
    #
    # In both case we have to ensure examples are not accidentally installed to $qt_prefix/bin or
    # similar.
    #
    # Example projects installation matrix.
    # 1) ExternalProject + unclean example install rules (INSTALL_EXAMPLEDIR is set) =>
    #    use _qt_internal_override_example_install_dir_to_dot + ExternalProject_Add's INSTALL_DIR
    #    using relative_dir from QT_EXAMPLE_BASE_DIR to example_source_dir
    #
    # 2) ExternalProject + clean example install rules =>
    #    use ExternalProject_Add's INSTALL_DIR using relative_dir from QT_EXAMPLE_BASE_DIR to
    #    example_source_dir, _qt_internal_override_example_install_dir_to_dot would be a no-op
    #
    # 3) in-tree + unclean example install rules (INSTALL_EXAMPLEDIR is set)
    # +
    # 4) in-tree + clean example install rules =>
    #    ensure CMAKE_INSTALL_PREFIX is unset in parent cmake_install.cmake file, set non-cache
    #    CMAKE_INSTALL_PREFIX using relative_dir from QT_EXAMPLE_BASE_DIR to
    #    example_source_dir, use _qt_internal_override_example_install_dir_to_dot to ensure
    #    INSTALL_EXAMPLEDIR does not interfere.

    # Allow customizing the installation path of the examples. Will be used in CI.
    if(QT_INTERNAL_EXAMPLES_INSTALL_PREFIX)
        set(qt_example_install_prefix "${QT_INTERNAL_EXAMPLES_INSTALL_PREFIX}")
    else()
        set(qt_example_install_prefix "${CMAKE_INSTALL_PREFIX}/${INSTALL_EXAMPLESDIR}")
    endif()
    file(TO_CMAKE_PATH "${qt_example_install_prefix}" qt_example_install_prefix)

    set(example_install_prefix "${qt_example_install_prefix}/${example_rel_path}")

    set(ep_binary_dir    "${CMAKE_CURRENT_BINARY_DIR}/${subdir}")

    set(build_command "")
    if(QT_INTERNAL_VERBOSE_EXAMPLES AND CMAKE_GENERATOR MATCHES "Ninja")
        set(build_command BUILD_COMMAND "${CMAKE_COMMAND}" --build "." -- -v)
    endif()

    ExternalProject_Add(${arg_NAME}
        EXCLUDE_FROM_ALL TRUE
        SOURCE_DIR       "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}"
        PREFIX           "${CMAKE_CURRENT_BINARY_DIR}/${subdir}-ep"
        STAMP_DIR        "${CMAKE_CURRENT_BINARY_DIR}/${subdir}-ep/stamp"
        BINARY_DIR       "${ep_binary_dir}"
        INSTALL_DIR      "${example_install_prefix}"
        INSTALL_COMMAND  ""
        ${build_command}
        TEST_COMMAND     ""
        DEPENDS          ${deps}
        CMAKE_CACHE_ARGS ${var_defs}
                         -DCMAKE_INSTALL_PREFIX:STRING=<INSTALL_DIR>
                         -DQT_INTERNAL_SET_EXAMPLE_INSTALL_DIR_TO_DOT:BOOL=TRUE
        ${terminal_args}
    )

    # Install the examples when the the user runs 'make install', and not at build time (which is
    # the default for ExternalProjects).
    install(CODE "\
# Install example from inside ExternalProject into the main build's install prefix.
execute_process(
    COMMAND
        \"${CMAKE_COMMAND}\" --build \"${ep_binary_dir}\" --target install
)
")

    # Force configure step to re-run after we configure the main project
    set(reconfigure_check_file ${CMAKE_CURRENT_BINARY_DIR}/reconfigure_${arg_NAME}.txt)
    file(TOUCH ${reconfigure_check_file})
    ExternalProject_Add_Step(${arg_NAME} reconfigure-check
        DEPENDERS configure
        DEPENDS   ${reconfigure_check_file}
        ${independent_args}
    )

    # Create an apk external project step and custom target that invokes the apk target
    # within the external project.
    # Make the global apk target depend on that custom target.
    if(ANDROID)
        ExternalProject_Add_Step(${arg_NAME} apk
            COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --target apk
            DEPENDEES configure
            EXCLUDE_FROM_MAIN YES
            ${terminal_args}
        )
        ExternalProject_Add_StepTargets(${arg_NAME} apk)

        if(TARGET apk)
            add_dependencies(apk ${arg_NAME}-apk)
        endif()
    endif()

    cmake_policy(POP)

    string(TOLOWER ${PROJECT_NAME} project_name_lower)
    add_dependencies(examples_${project_name_lower} ${arg_NAME})

endfunction()

if ("STANDALONE_TEST" IN_LIST Qt6BuildInternals_FIND_COMPONENTS)
    include(${CMAKE_CURRENT_LIST_DIR}/QtStandaloneTestTemplateProject/Main.cmake)
    if (NOT PROJECT_VERSION_MAJOR)
        get_property(_qt_major_version TARGET ${QT_CMAKE_EXPORT_NAMESPACE}::Core PROPERTY INTERFACE_QT_MAJOR_VERSION)
        set(PROJECT_VERSION ${Qt${_qt_major_version}Core_VERSION})

        string(REPLACE "." ";" _qt_core_version_list ${PROJECT_VERSION})
        list(GET _qt_core_version_list 0 PROJECT_VERSION_MAJOR)
        list(GET _qt_core_version_list 1 PROJECT_VERSION_MINOR)
        list(GET _qt_core_version_list 2 PROJECT_VERSION_PATCH)
    endif()
endif()

function(qt_internal_static_link_order_test)
    # The CMake versions greater than 3.21 take care about the resource object files order in a
    # linker line, it's expected that all object files are located at the beginning of the linker
    # line.
    # No need to run the test.
    if(CMAKE_VERSION VERSION_LESS 3.21)
        __qt_internal_check_link_order_matters(link_order_matters)
        if(link_order_matters)
            set(summary_message "no")
        else()
            set(summary_message "yes")
        endif()
    else()
        set(summary_message "yes")
    endif()
    qt_configure_add_summary_entry(TYPE "message"
        ARGS "Linker can resolve circular dependencies"
        MESSAGE "${summary_message}"
    )
endfunction()

function(qt_internal_check_cmp0099_available)
    # Don't care about CMP0099 in CMake versions greater than or equal to 3.21
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.21)
        return()
    endif()

    __qt_internal_check_cmp0099_available(result)
    if(result)
        set(summary_message "yes")
    else()
        set(summary_message "no")
    endif()
    qt_configure_add_summary_entry(TYPE "message"
        ARGS "CMake policy CMP0099 is supported"
        MESSAGE "${summary_message}"
    )
endfunction()

function(qt_internal_run_common_config_tests)
    qt_configure_add_summary_section(NAME "Common build options")
    qt_internal_static_link_order_test()
    qt_internal_check_cmp0099_available()
    qt_configure_end_summary_section()
endfunction()

# It is used in QtWebEngine to replace the REALPATH with ABSOLUTE path, which is
# useful for building Qt in Homebrew.
function(qt_internal_get_filename_path_mode out_var)
    set(mode REALPATH)
    if(APPLE AND QT_ALLOW_SYMLINK_IN_PATHS)
        set(mode ABSOLUTE)
    endif()
    set(${out_var} ${mode} PARENT_SCOPE)
endfunction()
