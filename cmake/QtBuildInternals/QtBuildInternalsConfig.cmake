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

macro(qt_build_repo_begin)
    if(${ARGC} EQUAL 1 AND "${ARGV0}" STREQUAL "SKIP_CMAKE_MODULE_PATH_ADDITION")
        # No-op.
    else()
        # Set up the paths for the modules.
        set(QT_CMAKE_MODULE_PATH "${QT_BUILD_INTERNALS_PATH}/../${QT_CMAKE_EXPORT_NAMESPACE}")
        list(APPEND CMAKE_MODULE_PATH ${QT_CMAKE_MODULE_PATH})

        # If the repo has its own cmake modules, include those in the module path.
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
            list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
        endif()
    endif()

    # Qt specific setup common for all modules:
    include(QtSetup)
    include(FeatureSummary)
endmacro()

macro(qt_build_repo_end)
    # Delayed actions on some of the Qt targets:
    include(QtPostProcess)

    # Install the repo-specific cmake find modules.
    qt_path_join(__qt_repo_install_dir ${QT_CONFIG_INSTALL_DIR} ${INSTALL_CMAKE_NAMESPACE})

    if(NOT PROJECT_NAME STREQUAL "QtBase")
        qt_copy_or_install(DIRECTORY cmake/
            DESTINATION "${__qt_repo_install_dir}"
            FILES_MATCHING PATTERN "Find*.cmake"
        )
    endif()

    # Print a feature summary:
    feature_summary(WHAT PACKAGES_FOUND
                         REQUIRED_PACKAGES_NOT_FOUND
                         RECOMMENDED_PACKAGES_NOT_FOUND
                         OPTIONAL_PACKAGES_NOT_FOUND
                         RUNTIME_PACKAGES_NOT_FOUND
                         FATAL_ON_MISSING_REQUIRED_PACKAGES)
endmacro()
