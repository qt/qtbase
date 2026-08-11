# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


find_package(Qt6 REQUIRED COMPONENTS Core)

set(QT_NO_PACKAGE_VERSION_CHECK FALSE)

include(CMakePackageConfigHelpers)

set(QT_CMAKE_DIR "${Qt6_DIR}")
include("${Qt6_DIR}/QtCMakeHelpers.cmake")

set(mock_pkg_name "TestPkg")
set(mock_pkg_dir "${CMAKE_CURRENT_BINARY_DIR}/mockpkg")

# Generates TestPkgConfig.cmake with logic mirroring the 'umbrella' component loop in
# the real Qt6Config.cmake: for every requested component, find_package() it
# unless a dependency of an earlier component already pulled it in; then
# make sure whatever ended up FOUND actually satisfies the version that was
# requested directly.
function(setup_mock_package version)
    file(REMOVE_RECURSE "${mock_pkg_dir}")
    file(MAKE_DIRECTORY "${mock_pkg_dir}")

    write_basic_package_version_file(
        "${mock_pkg_dir}/${mock_pkg_name}ConfigVersionImpl.cmake"
        VERSION ${version}
        COMPATIBILITY AnyNewerVersion
    )

    qt_internal_write_qt_package_version_file(
        ${mock_pkg_name}
        "${mock_pkg_dir}/${mock_pkg_name}ConfigVersion.cmake"
        ALLOW_OVERRIDE_FILES
    )

    file(WRITE "${mock_pkg_dir}/${mock_pkg_name}Config.cmake" "
set(${mock_pkg_name}_FOUND TRUE)
foreach(__qt_test_module IN LISTS ${mock_pkg_name}_FIND_COMPONENTS)
    if(NOT ${mock_pkg_name}\${__qt_test_module}_FOUND)
        find_package(${mock_pkg_name}\${__qt_test_module} \${${mock_pkg_name}_FIND_VERSION} QUIET
            PATHS \"${mock_pkg_dir}\"
            NO_DEFAULT_PATH
        )
    endif()
    if(NOT ${mock_pkg_name}\${__qt_test_module}_FOUND)
        set(${mock_pkg_name}_FOUND FALSE)
    endif()
    unset(__qt_test_module)
endforeach()
")
endfunction()

# Generates a plain mock module version file the way a regular Qt module is shipped
# ie. TestPkg<modle>Config.cmake
# that includes its TestPkg<moduile>Dependencies.cmake if present plus a regular
# TestPkg<module>ConfigVersion.cmake produced by write_basic_package_version_file()
function(setup_mock_module_package module version)
    set(prefix "${mock_pkg_dir}/${mock_pkg_name}${module}")

    file(WRITE "${prefix}Config.cmake" "
if(EXISTS \"\${CMAKE_CURRENT_LIST_DIR}/${mock_pkg_name}${module}Dependencies.cmake\")
    include(\"\${CMAKE_CURRENT_LIST_DIR}/${mock_pkg_name}${module}Dependencies.cmake\")
endif()
if(NOT DEFINED \"${mock_pkg_name}${module}_FOUND\")
    set(\"${mock_pkg_name}${module}_FOUND\" TRUE)
endif()
")
    write_basic_package_version_file(
        "${prefix}ConfigVersion.cmake"
        VERSION ${version}
        COMPATIBILITY AnyNewerVersion
    )
endfunction()

function(write_override_file component filename content)
    set(dir "${mock_pkg_dir}/ConfigVersionOverrides/${component}")
    file(MAKE_DIRECTORY "${dir}")
    file(WRITE "${dir}/${filename}" "${content}")
endfunction()

function(create_empty_override_dir component)
    file(MAKE_DIRECTORY "${mock_pkg_dir}/ConfigVersionOverrides/${component}")
endfunction()

# Generates TestPkg<module>Dependencies.cmake, the same way
# QtModuleDependencies.cmake.in generates e.g. Qt6WidgetsDependencies.cmake:
# a "pkg;version" pair is resolved via the real
# _qt_internal_find_qt_dependencies() helper (QtPublicDependencyHelpers.cmake),
# which in turn calls find_dependency() for it.
function(write_module_dependency module dep_pkg dep_version)
    set(prefix "${mock_pkg_dir}/${mock_pkg_name}${module}")

    file(WRITE "${prefix}Dependencies.cmake" "
include(CMakeFindDependencyMacro)
set(__qt_test_${module}_target_deps \"${dep_pkg}\\;${dep_version}\")
set(__qt_test_${module}_find_dependency_paths \"${mock_pkg_dir}\")
_qt_internal_find_qt_dependencies(${module}
    __qt_test_${module}_target_deps
    __qt_test_${module}_find_dependency_paths
)
")
endfunction()


