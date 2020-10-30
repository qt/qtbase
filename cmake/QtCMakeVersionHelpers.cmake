# Returns the minimum supported CMake version required to build Qt as originally advertised by Qt.
function(qt_internal_get_qt_supported_minimum_cmake_version out_var)
    # QT_MIN_SUPPORTED_CMAKE_VERSION is set either in .cmake.conf or in QtBuildInternalsExtras.cmake
    # when building a child repo.
    set(supported_version "${QT_MIN_SUPPORTED_CMAKE_VERSION}")
    set(${out_var} "${supported_version}" PARENT_SCOPE)
endfunction()

# Returns the computed minimum supported CMake version required to build Qt.
function(qt_internal_get_computed_minimum_cmake_version out_var)
    qt_internal_get_qt_supported_minimum_cmake_version(min_supported_version)
    set(computed_min_version "${min_supported_version}")

    # Set in QtBuildInternalsExtras.cmake.
    if(QT_COMPUTED_MIN_CMAKE_VERSION)
        set(computed_min_version "${QT_COMPUTED_MIN_CMAKE_VERSION}")
    endif()

    # An explicit override for those that take it upon themselves to fix the build system
    # when using a CMake version lower than the one officially supported.
    # Also useful for build testing locally with different minimum versions to observe different
    # policy behaviors.
    if(QT_FORCE_MIN_CMAKE_VERSION)
        set(computed_min_version "${QT_FORCE_MIN_CMAKE_VERSION}")
    endif()

    set(${out_var} "${computed_min_version}" PARENT_SCOPE)
endfunction()

function(qt_internal_check_for_suitable_cmake_version)
    # Implementation note.
    # The very first cmake_required_version() call can't be placed in an include()d file.
    # It causes CMake to fail to configure with 'No cmake_minimum_required command is present.'
    # The first cmake_required_version() must be called directly in the top-level CMakeLists.txt
    # file.
    # That's why this function only handles output of warnings, and doesn't try to set the required
    # version.
    qt_internal_check_minimum_cmake_version()
    qt_internal_warn_about_unsuitable_cmake_versions()
endfunction()

# Function to be used in child repos like qtsvg to require a minimum CMake version.
#
# Such repos don't have the required version information at cmake_required_version() time, that's
# why we provide this function to be called at a time when the info is available.
function(qt_internal_require_suitable_cmake_version)
    qt_internal_check_for_suitable_cmake_version()
    qt_internal_get_computed_minimum_cmake_version(computed_min_version)

    if(CMAKE_VERSION VERSION_LESS computed_min_version)
        message(FATAL_ERROR "CMake ${computed_min_version} or higher is required. "
                            "You are running version ${CMAKE_VERSION}")
    endif()
endfunction()

function(qt_internal_check_minimum_cmake_version)
    qt_internal_get_qt_supported_minimum_cmake_version(min_supported_version)
    qt_internal_get_computed_minimum_cmake_version(computed_min_version)

    if(NOT min_supported_version STREQUAL computed_min_version
            AND computed_min_version VERSION_LESS min_supported_version)
        message(WARNING
               "The minimum required CMake version to build Qt is '${min_supported_version}'. "
               "You have explicitly chosen to require a lower minimum CMake version, namely '${computed_min_version}'. "
               "Building Qt with such a CMake version is not officially supported. Use at your own risk.")
    endif()
endfunction()

function(qt_internal_warn_about_unsuitable_cmake_versions)
    set(unsuitable_versions "")

    # Touching a library's source file causes unnecessary rebuilding of unrelated targets.
    # https://gitlab.kitware.com/cmake/cmake/-/issues/21020
    list(APPEND unsuitable_versions "3.18.0")

    # Ninja Multi-Config race condition overrides response files of different configurations
    # https://gitlab.kitware.com/cmake/cmake/-/issues/20961
    # https://gitlab.kitware.com/cmake/cmake/-/issues/21050 (follow up issue)
    list(APPEND unsuitable_versions "3.18.1")

    # AUTOMOC dependencies are not properly created when using Ninja Multi-Config.
    # https://gitlab.kitware.com/cmake/cmake/-/issues/21118
    #
    # Also until 3.18.2 inclusive, AUTOMOC dependencies are out-of-date if a previously header
    # disappears (for example when upgrading a compiler)
    # https://gitlab.kitware.com/cmake/cmake/-/issues/21136
    #
    # Also multi-arch PCH doesn't work on iOS. Can't quite find the original upstream CMake issue
    # but the Qt one was detected at https://codereview.qt-project.org/c/qt/qt5/+/310947
    # And a follow up issue regarding PCH and -fembed-bitcode failing.
    # https://gitlab.kitware.com/cmake/cmake/-/issues/21163
    list(APPEND unsuitable_versions "3.18.2")

    foreach(unsuitable_version ${unsuitable_versions})
        if(CMAKE_VERSION VERSION_EQUAL unsuitable_version)
            message(WARNING
                "The CMake version you are using is known to cause issues when building Qt. "
                "Please upgrade to a newer version. "
                "CMake version used: '${unsuitable_version}'")
        endif()
    endforeach()
endfunction()
