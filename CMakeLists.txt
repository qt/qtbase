# special case skip regeneration
cmake_minimum_required(VERSION 3.15.0)

# Run auto detection routines, but not when doing stnadalone tests. In that case the detection
# results are takend from either QtBuildInternals or the qt.toolchain.cmake file.
if(NOT QT_BUILD_STANDALONE_TESTS)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/QtAutoDetect.cmake)
endif()

include(".cmake.conf")
project(QtBase
    VERSION "${QT_REPO_MODULE_VERSION}"
    DESCRIPTION "Qt Base Libraries"
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C ASM
)

# Make sure we only use latest private CMake API, aka no compatibility wrappers.
set(QT_NO_INTERNAL_COMPATIBILITY_FUNCTIONS TRUE)

if (CMAKE_CROSSCOMPILING AND CMAKE_SYSROOT)
    # When cross compiling with CMake any calls to pkg_check_modules() will
    # search into the host system instead of the target sysroot.
    # The current work around is based on the discussion found at
    # https://gitlab.kitware.com/cmake/cmake/-/merge_requests/4478
    set(ENV{PKG_CONFIG_DIR} "")
    set(ENV{PKG_CONFIG_LIBDIR} "${CMAKE_SYSROOT}/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}/pkgconfig")
    set(ENV{PKG_CONFIG_SYSROOT_DIR} ${CMAKE_SYSROOT})
endif()

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

qt_build_repo_begin()

if(NOT QT_BUILD_STANDALONE_TESTS)
    ## Should this Qt be static or dynamically linked?
    option(BUILD_SHARED_LIBS "Build Qt statically or dynamically" ON)
    set(QT_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})

    ## Should this Qt be built with Werror?
    option(WARNINGS_ARE_ERRORS "Build Qt with warnings as errors" ${FEATURE_developer_build})

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
    include(src/corelib/Qt6AndroidMacros.cmake)

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

if(BUILD_TESTING)
    add_subdirectory(tests)
    if(QT_NO_MAKE_TESTS)
        set_property(DIRECTORY tests PROPERTY EXCLUDE_FROM_ALL TRUE)
    endif()
endif()

if(NOT QT_BUILD_STANDALONE_TESTS)
    if(QT_WILL_BUILD_TOOLS)
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
                               DESTINATION ${mkspecs_install_dir})
        else()
            qt_copy_or_install(FILES "${entry}"
                               DESTINATION ${mkspecs_install_dir})
        endif()
    endforeach()
endif()

qt_build_repo_end()

if(NOT QT_BUILD_STANDALONE_TESTS AND BUILD_EXAMPLES)
    add_subdirectory(examples)
    if(QT_NO_MAKE_EXAMPLES)
        set_property(DIRECTORY examples PROPERTY EXCLUDE_FROM_ALL TRUE)
    endif()
endif()
