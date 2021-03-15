# These values should be kept in sync with those in qtbase/.cmake.conf
cmake_minimum_required(VERSION 3.14...3.19)

######################################
#
#       Macros for building Qt modules
#
######################################

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

    if(APPLE OR WIN32 OR QNX OR ANDROID OR (NOT PKG_CONFIG_EXECUTABLE))
        set(pkg_config_enabled OFF)
    endif()

    # Features won't have been evaluated yet if this is the first run, have to evaluate this here
    if(NOT "${FEATURE_pkg_config}" AND "${INPUT_pkg_config}"
       AND NOT "${INPUT_pkg_config}" STREQUAL "undefined")
        set(FEATURE_pkg_config ON)
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

macro(qt_build_internals_set_up_private_api)
    # Check for the minimum CMake version.
    include(QtCMakeVersionHelpers)
    qt_internal_require_suitable_cmake_version()
    qt_internal_upgrade_cmake_policies()

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
    get_directory_property(_directories DIRECTORY "${subdir}" SUBDIRECTORIES)
    if (_directories)
        foreach(_directory IN LISTS _directories)
            get_directory_property(_targets DIRECTORY "${_directory}" BUILDSYSTEM_TARGETS)
            if (_targets)
                foreach(_target IN LISTS _targets)
                    get_target_property(_type ${_target} TYPE)
                    if (NOT (${_type} STREQUAL "UTILITY" OR ${_type} STREQUAL "INTERFACE"))
                        list(APPEND qt_repo_targets "${_target}")
                    endif()
                endforeach()
            endif()
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
                                DEPENDS ${qt_repo_targets}
                                COMMENT "Building everything in ${qt_repo_targets_name}/${qt_repo_target_basename}")
            list(APPEND qt_repo_target_all "${qt_repo_target_name}")
        endif()
    endforeach()
    if (qt_repo_target_all)
        add_custom_target("${qt_repo_targets_name}"
                            DEPENDS ${qt_repo_target_all}
                            COMMENT "Building everything in ${qt_repo_targets_name}")
    endif()
endfunction()

macro(qt_enable_cmake_languages)
    include(CheckLanguage)
    set(__qt_required_language_list C CXX)
    set(__qt_optional_language_list )

    # https://gitlab.kitware.com/cmake/cmake/-/issues/20545
    if(APPLE)
        list(APPEND __qt_optional_language_list OBJC OBJCXX)
    endif()

    foreach(__qt_lang ${__qt_required_language_list})
        enable_language(${__qt_lang})
    endforeach()

    foreach(__qt_lang ${__qt_optional_language_list})
        check_language(${__qt_lang})
        if(CMAKE_${__qt_lang}_COMPILER)
            enable_language(${__qt_lang})
        endif()
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

macro(qt_build_repo_begin)
    qt_build_internals_set_up_private_api()
    qt_enable_cmake_languages()

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
endmacro()

macro(qt_build_repo_end)
    if(NOT QT_BUILD_STANDALONE_TESTS)
        # Delayed actions on some of the Qt targets:
        include(QtPostProcess)

        # Install the repo-specific cmake find modules.
        qt_path_join(__qt_repo_install_dir ${QT_CONFIG_INSTALL_DIR} ${INSTALL_CMAKE_NAMESPACE})

        if(NOT PROJECT_NAME STREQUAL "QtBase")
            if (EXISTS cmake)
                qt_copy_or_install(DIRECTORY cmake/
                    DESTINATION "${__qt_repo_install_dir}"
                    FILES_MATCHING PATTERN "Find*.cmake"
                )
            endif()
        endif()

        if(NOT QT_SUPERBUILD)
            qt_print_feature_summary()
        endif()
    endif()

    if(NOT QT_SUPERBUILD)
        qt_print_build_instructions()
    else()
        qt_build_internals_add_toplevel_targets()
    endif()
endmacro()

macro(qt_build_repo)
    qt_build_repo_begin(${ARGN})

    # If testing is enabled, try to find the qtbase Test package.
    # Do this before adding src, because there might be test related conditions
    # in source.
    if (QT_BUILD_TESTS AND NOT QT_BUILD_STANDALONE_TESTS)
        find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS Test)
    endif()

    if(NOT QT_BUILD_STANDALONE_TESTS)
        if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/CMakeLists.txt")
            add_subdirectory(src)
        endif()

        if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tools/CMakeLists.txt")
            add_subdirectory(tools)
        endif()
    endif()

    if (QT_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
        add_subdirectory(tests)
        if(NOT QT_BUILD_TESTS_BY_DEFAULT)
            set_property(DIRECTORY tests PROPERTY EXCLUDE_FROM_ALL TRUE)
        endif()
    endif()

    qt_build_repo_end()

    if(QT_BUILD_EXAMPLES AND BUILD_SHARED_LIBS
            AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt"
            AND NOT QT_BUILD_STANDALONE_TESTS)
        add_subdirectory(examples)
        if(NOT QT_BUILD_EXAMPLES_BY_DEFAULT)
            set_property(DIRECTORY examples PROPERTY EXCLUDE_FROM_ALL TRUE)
        endif()
    endif()
endmacro()

macro(qt_set_up_standalone_tests_build)
    # Remove this macro once all usages of it have been removed.
    # Standalone tests are not handled via the main repo project and qt_build_tests.
endmacro()

function(qt_get_standalone_tests_confg_files_path out_var)
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

macro(qt_build_tests)
    if(QT_BUILD_STANDALONE_TESTS)
        # Find location of TestsConfig.cmake. These contain the modules that need to be
        # find_package'd when testing.
        qt_get_standalone_tests_confg_files_path(_qt_build_tests_install_prefix)
        include("${_qt_build_tests_install_prefix}/${PROJECT_NAME}TestsConfig.cmake" OPTIONAL)

        # Of course we always need the test module as well.
        find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS Test)

        # Set language standards after finding Core, because that's when the relevant
        # feature variables are available, and the call in QtSetup is too early when building
        # standalone tests, because Core was not find_package()'d yet.
        qt_set_language_standards()

        if(NOT QT_SUPERBUILD)
            # Set up fake standalone tests install prefix, so we don't pollute the Qt install
            # prefix. For super builds it needs to be done in qt5/CMakeLists.txt.
            qt_set_up_fake_standalone_tests_install_prefix()
        endif()
    endif()

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/auto/CMakeLists.txt")
        add_subdirectory(auto)
    endif()
    if(NOT QT_BUILD_MINIMAL_STATIC_TESTS)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/CMakeLists.txt" AND QT_BUILD_BENCHMARKS)
            add_subdirectory(benchmarks)
        endif()
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/manual/CMakeLists.txt" AND QT_BUILD_MANUAL_TESTS)
            add_subdirectory(manual)
        endif()
    endif()
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
    if(QT_WILL_INSTALL)
        get_filename_component(clean_config_prefix
                               "${CMAKE_INSTALL_PREFIX}/${QT_CONFIG_INSTALL_DIR}" ABSOLUTE)
    else()
        get_filename_component(clean_config_prefix "${QT_CONFIG_BUILD_DIR}" ABSOLUTE)
    endif()
    file(RELATIVE_PATH
         qt_path_from_cmake_config_dir_to_prefix
         "${clean_config_prefix}" "${CMAKE_INSTALL_PREFIX}")
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
    list(APPEND CMAKE_PREFIX_PATH "${QT_BUILD_DIR}")
    # Make sure the CMake config files do not recreate the already-existing targets
    set(QT_NO_CREATE_TARGETS TRUE)
    set(BACKUP_CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ${CMAKE_FIND_ROOT_PATH_MODE_PACKAGE})
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE "BOTH")
endmacro()

macro(qt_examples_build_begin)
    # Examples that are built as part of the Qt build need to use the CMake config files from the
    # build dir, because they are not installed yet in a prefix build.
    # Appending to CMAKE_PREFIX_PATH helps find the initial Qt6Config.cmake.
    # Appending to QT_EXAMPLES_CMAKE_PREFIX_PATH helps find components of Qt6, because those
    # find_package calls use NO_DEFAULT_PATH, and thus CMAKE_PREFIX_PATH is ignored.
    qt_internal_set_up_build_dir_package_paths()
    list(APPEND QT_EXAMPLES_CMAKE_PREFIX_PATH "${QT_BUILD_DIR}")

    # Because CMAKE_INSTALL_RPATH is empty by default in the repo project, examples need to have
    # it set here, so they can run when installed.
    # This means that installed examples are not relocatable at the moment. We would need to
    # annotate where each example is installed to, to be able to derive a relative rpath, and it
    # seems there's no way to query such information from CMake itself.
    set(CMAKE_INSTALL_RPATH "${_default_install_rpath}")
    set(QT_DISABLE_QT_ADD_PLUGIN_COMPATIBILITY TRUE)
endmacro()

macro(qt_examples_build_end)
    # We use AUTOMOC/UIC/RCC in the examples. Make sure to not fail on a fresh Qt build, that e.g. the moc binary does not exist yet.

    # This function gets all targets below this directory
    function(get_all_targets _result _dir)
        get_property(_subdirs DIRECTORY "${_dir}" PROPERTY SUBDIRECTORIES)
        foreach(_subdir IN LISTS _subdirs)
            get_all_targets(${_result} "${_subdir}")
        endforeach()
        get_property(_sub_targets DIRECTORY "${_dir}" PROPERTY BUILDSYSTEM_TARGETS)
        set(${_result} ${${_result}} ${_sub_targets} PARENT_SCOPE)
    endfunction()

    get_all_targets(targets "${CMAKE_CURRENT_SOURCE_DIR}")

    foreach(target ${targets})
        qt_autogen_tools(${target} ENABLE_AUTOGEN_TOOLS "moc" "rcc")
        if(TARGET Qt::Widgets)
            qt_autogen_tools(${target} ENABLE_AUTOGEN_TOOLS "uic")
        endif()
    endforeach()

    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ${BACKUP_CMAKE_FIND_ROOT_PATH_MODE_PACKAGE})
endmacro()
