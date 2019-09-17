if (CMAKE_VERSION VERSION_LESS 3.1.0)
    message(FATAL_ERROR "Qt requires at least CMake version 3.1.0")
endif()

######################################
#
#       Macros for building Qt modules
#
######################################

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/QtBuildInternalsExtra.cmake")
    include(${CMAKE_CURRENT_LIST_DIR}/QtBuildInternalsExtra.cmake)
endif()

macro(qt_set_up_build_internals_paths)
    # Set up the paths for the cmake modules located in the build dir. Prepend, so the paths are
    # least important compared to the source dir ones, but more important than command line
    # provided ones.
    set(QT_CMAKE_MODULE_PATH "${QT_BUILD_INTERNALS_PATH}/../${QT_CMAKE_EXPORT_NAMESPACE}")
    list(PREPEND CMAKE_MODULE_PATH "${QT_CMAKE_MODULE_PATH}")

    # When doing a non-prefix build, prepend the qtbase source cmake directory to CMAKE_MODULE_PATH,
    # so that if a change is done in cmake/QtBuild.cmake, it gets automatically picked up when
    # building qtdeclarative, rather than having to build qtbase first (which will copy
    # QtBuild.cmake to the build dir). This is similar to qmake non-prefix builds, where the
    # source qtbase/mkspecs directory is used.
    if(NOT QT_WILL_INSTALL)
        list(PREPEND CMAKE_MODULE_PATH "${QT_SOURCE_TREE}/cmake")
    endif()

    # If the repo has its own cmake modules, include those in the module path.
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
        list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
    endif()
endmacro()

macro(qt_build_repo_begin)
    if(${ARGC} EQUAL 1 AND "${ARGV0}" STREQUAL "SKIP_CMAKE_MODULE_PATH_ADDITION")
        # No-op.
    else()
        qt_set_up_build_internals_paths()
    endif()

    # Qt specific setup common for all modules:
    include(QtSetup)
    include(FeatureSummary)

    # Optionally include a repo specific Setup module.
    include(${PROJECT_NAME}Setup OPTIONAL)

    # Find Apple frameworks if needed.
    qt_find_apple_system_frameworks()
endmacro()

macro(qt_build_repo_end)
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

    # Print a feature summary:
    feature_summary(WHAT PACKAGES_FOUND
                         REQUIRED_PACKAGES_NOT_FOUND
                         RECOMMENDED_PACKAGES_NOT_FOUND
                         OPTIONAL_PACKAGES_NOT_FOUND
                         RUNTIME_PACKAGES_NOT_FOUND
                         FATAL_ON_MISSING_REQUIRED_PACKAGES)

    qt_print_build_instructions()
endmacro()

function(qt_print_build_instructions)
    if(NOT PROJECT_NAME STREQUAL "QtBase")
        return()
    endif()

    set(build_command "cmake --build . --parallel")
    set(install_command "cmake --install .")

    message("Qt is now configured for building. Just run '${build_command}'.")
    if(QT_WILL_INSTALL)
        message("Once everything is built, you must run '${install_command}'.")
        message("Qt will be installed into '${CMAKE_INSTALL_PREFIX}'")
    else()
        message("Once everything is built, Qt is installed.")
        message("You should NOT run '${install_command}'")
        message("Note that this build cannot be deployed to other machines or devices.")
    endif()
    message("To configure and build other modules, you can use the following convenience script:
        ${CMAKE_INSTALL_PREFIX}/${INSTALL_BINDIR}/qt-cmake")
    message("\nIf reconfiguration fails for some reason, try to remove 'CMakeCache.txt' \
from the build directory \n")
endfunction()

macro(qt_build_repo)
    qt_build_repo_begin(${ARGN})

    # If testing is enabled, try to find the qtbase Test package.
    # Do this before adding src, because there might be test related conditions
    # in source.
    if (BUILD_TESTING)
        find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS Test)
    endif()

    ## Decide whether tools will be built.
    qt_check_if_tools_will_be_built()

    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/CMakeLists.txt")
        add_subdirectory(src)
    endif()

    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tools/CMakeLists.txt")
        add_subdirectory(tools)
    endif()

    if (BUILD_TESTING AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
        add_subdirectory(tests)
    endif()

    qt_build_repo_end()

    if (BUILD_EXAMPLES AND BUILD_SHARED_LIBS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt")
        add_subdirectory(examples)
    endif()
endmacro()

macro(qt_set_up_standalone_tests_build)
    qt_set_up_build_internals_paths()
    include(QtSetup)
    qt_find_apple_system_frameworks()
    qt_check_if_tools_will_be_built()
endmacro()

macro(qt_build_tests)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/auto/CMakeLists.txt")
        add_subdirectory(auto)
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/CMakeLists.txt")
        add_subdirectory(benchmarks)
    endif()
endmacro()

macro(qt_examples_build_begin)
    # It is part of a Qt build => Use the CMake config files from the binary dir
    list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}")
    # Also make sure the CMake config files do not recreate the already-existing targets
    set(QT_NO_CREATE_TARGETS TRUE)
    set(BACKUP_CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ${CMAKE_FIND_ROOT_PATH_MODE_PACKAGE})
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE "BOTH")
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
        qt_autogen_tools(${target} ENABLE_AUTOGEN_TOOLS "moc" "uic" "rcc")
    endforeach()

    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ${BACKUP_CMAKE_FIND_ROOT_PATH_MODE_PACKAGE})
endmacro()

if (ANDROID)
    include(${CMAKE_CURRENT_LIST_DIR}/QtBuildInternalsAndroid.cmake)
endif()
