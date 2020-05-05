if (CMAKE_VERSION VERSION_LESS 3.1.0)
    message(FATAL_ERROR "Qt requires at least CMake version 3.1.0")
endif()

######################################
#
#       Macros for building Qt modules
#
######################################

set(QT_BACKUP_CMAKE_INSTALL_PREFIX_BEFORE_EXTRA_INCLUDE "${CMAKE_INSTALL_PREFIX}")

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/QtBuildInternalsExtra.cmake")
    include(${CMAKE_CURRENT_LIST_DIR}/QtBuildInternalsExtra.cmake)
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

macro(qt_build_internals_set_up_private_api)
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
        add_custom_target(install_html_docs_docs)
        add_custom_target(install_qch_docs_docs)
        add_custom_target(install_docs_docs)
    endif()

    string(TOLOWER ${PROJECT_NAME} project_name_lower)

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
    add_dependencies(${qt_docs_install_html_target_name} ${qt_docs_html_target_name})
    add_dependencies(${qt_docs_install_qch_target_name} ${qt_docs_qch_target_name})
    add_dependencies(${qt_docs_install_target_name} ${qt_docs_install_html_target_name} ${qt_docs_install_qch_target_name})

    # Make global doc targets depend on the module ones.
    add_dependencies(docs ${qt_docs_target_name})
    add_dependencies(prepare_docs ${qt_docs_prepare_target_name})
    add_dependencies(generate_docs ${qt_docs_generate_target_name})
    add_dependencies(html_docs ${qt_docs_html_target_name})
    add_dependencies(qch_docs ${qt_docs_qch_target_name})
    add_dependencies(install_html_docs_docs ${qt_docs_install_html_target_name})
    add_dependencies(install_qch_docs_docs ${qt_docs_install_qch_target_name})
    add_dependencies(install_docs_docs ${qt_docs_install_target_name})
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
    endif()
endmacro()

macro(qt_build_repo)
    qt_build_repo_begin(${ARGN})

    # If testing is enabled, try to find the qtbase Test package.
    # Do this before adding src, because there might be test related conditions
    # in source.
    if (BUILD_TESTING AND NOT QT_BUILD_STANDALONE_TESTS)
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

    if (BUILD_TESTING AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
        add_subdirectory(tests)
        if(QT_NO_MAKE_TESTS)
            set_property(DIRECTORY tests PROPERTY EXCLUDE_FROM_ALL TRUE)
        endif()
    endif()

    qt_build_repo_end()

    if (BUILD_EXAMPLES AND BUILD_SHARED_LIBS
            AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt"
            AND NOT QT_BUILD_STANDALONE_TESTS)
        add_subdirectory(examples)
        if(QT_NO_MAKE_EXAMPLES)
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
    set("${out_var}" "${path}" PARENT_SCOPE)
endfunction()

macro(qt_build_tests)
    if(QT_BUILD_STANDALONE_TESTS)
        # Find location of TestsConfig.cmake. These contain the modules that need to be
        # find_package'd when testing.
        qt_get_standalone_tests_confg_files_path(_qt_build_tests_install_prefix)
        if(QT_WILL_INSTALL)
            qt_path_join(_qt_build_tests_install_prefix
                         ${CMAKE_INSTALL_PREFIX} ${_qt_build_tests_install_prefix})
        endif()
        include("${_qt_build_tests_install_prefix}/${PROJECT_NAME}TestsConfig.cmake" OPTIONAL)

        # Of course we always need the test module as well.
        find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS Test)

        # Set language standards after finding Core, because that's when the relevant
        # feature variables are available, and the call in QtSetup is too early when building
        # standalone tests, because Core was not find_package()'d yet.
        qt_set_language_standards()

        if(NOT QT_SUPERBUILD)
            # Restore original install prefix. For super builds it needs to be done in
            # qt5/CMakeLists.txt.
            qt_restore_backed_up_install_prefix()
        endif()
    endif()

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/auto/CMakeLists.txt")
        add_subdirectory(auto)
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/CMakeLists.txt" AND QT_BUILD_BENCHMARKS)
        add_subdirectory(benchmarks)
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/manual/CMakeLists.txt")
        # add_subdirectory(manual) don't build manual tests for now, because qmake doesn't.
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
         ${clean_config_prefix} ${CMAKE_INSTALL_PREFIX})
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

function(qt_restore_backed_up_install_prefix)
    # Restore the CMAKE_INSTALL_PREFIX that was set before loading BuildInternals.
    # Useful for standalone tests, we don't want to accidentally install a test into the Qt prefix.
    get_property(helpstring CACHE CMAKE_INSTALL_PREFIX PROPERTY HELPSTRING)

    # If CMAKE_INSTALL_PREFIX was default initialized, that means it points to something
    # like /usr/local which we don't want. Why? When metatype json files are created
    # during standalone tests configuration, the folder creation might fail due to missing
    # permissions in the /usr/local (which is the wrong place anyway).
    # Instead specify a dummy install prefix in the current build dir.
    # If the prefix was specified by the user at the command line, honor it, hoping that the
    # user knows what they are doing.
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(new_install_prefix "${CMAKE_BINARY_DIR}/standalone_tests_fake_install_prefix")
    else()
        set(new_install_prefix "${QT_BACKUP_CMAKE_INSTALL_PREFIX_BEFORE_EXTRA_INCLUDE}")
    endif()

    set(CMAKE_INSTALL_PREFIX "${new_install_prefix}" CACHE STRING "${helpstring}" FORCE)
endfunction()

macro(qt_examples_build_begin)
    # Examples that are built as part of the Qt build need to use the CMake config files from the
    # build dir, because they are not installed yet in a prefix build.
    # Appending to CMAKE_PREFIX_PATH helps find the initial Qt6Config.cmake.
    # Appending to QT_EXAMPLES_CMAKE_PREFIX_PATH helps find components of Qt6, because those
    # find_package calls use NO_DEFAULT_PATH, and thus CMAKE_PREFIX_PATH is ignored.
    list(APPEND CMAKE_PREFIX_PATH "${QT_BUILD_DIR}")
    list(APPEND QT_EXAMPLES_CMAKE_PREFIX_PATH "${QT_BUILD_DIR}")
    # Also make sure the CMake config files do not recreate the already-existing targets
    set(QT_NO_CREATE_TARGETS TRUE)
    set(BACKUP_CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ${CMAKE_FIND_ROOT_PATH_MODE_PACKAGE})
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE "BOTH")

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

if (ANDROID)
    include(${CMAKE_CURRENT_LIST_DIR}/QtBuildInternalsAndroid.cmake)
endif()
