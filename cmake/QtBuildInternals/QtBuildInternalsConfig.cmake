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
    # Set up the paths for the modules.
    set(QT_CMAKE_MODULE_PATH "${QT_BUILD_INTERNALS_PATH}/../${QT_CMAKE_EXPORT_NAMESPACE}")
    list(APPEND CMAKE_MODULE_PATH ${QT_CMAKE_MODULE_PATH})

    # If the repo has its own cmake modules, include those in the module path.
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
        list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
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
endmacro()

macro(qt_build_repo)
    qt_build_repo_begin(${ARGN})

    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/CMakeLists.txt")
        add_subdirectory(src)
    endif()

    if (BUILD_TESTING AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
        find_package(Qt5 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS Test Xml)
        add_subdirectory(tests)
    endif()

    if (BUILD_EXAMPLES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt")
        add_subdirectory(examples)
    endif()

    qt_build_repo_end()
endmacro()

macro(qt_set_up_standalone_tests_build)
    qt_set_up_build_internals_paths()
    include(QtSetup)
    qt_find_apple_system_frameworks()
endmacro()

macro(qt_build_tests)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/auto/CMakeLists.txt")
        add_subdirectory(auto)
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/CMakeLists.txt")
        add_subdirectory(benchmarks)
    endif()
endmacro()
