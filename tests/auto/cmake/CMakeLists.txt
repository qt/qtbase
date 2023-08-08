# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# This is an automatic test for the CMake configuration files.
# To run it manually,
# 1) mkdir build   # Create a build directory
# 2) cd build
# 3) # Run cmake on this directory
#    `$qt_prefix/bin/qt-cmake ..` or `cmake -DCMAKE_PREFIX_PATH=/path/to/qt ..`
# 4) ctest         # Run ctest
# 5) ctest -V -R test_wrap_cpp_options # Run single test
#
# The expected output is something like:
#
#       Start  1: test_use_modules_function
#  1/11 Test  #1: test_use_modules_function ........   Passed    3.36 sec
#       Start  2: test_wrap_cpp_and_resources
#  2/11 Test  #2: test_wrap_cpp_and_resources ......   Passed    1.41 sec
#       Start  3: test_dependent_modules
#  3/11 Test  #3: test_dependent_modules ...........   Passed    2.22 sec
#       Start  4: test_add_resource_options
#  4/11 Test  #4: test_add_resource_options ........   Passed    0.16 sec
#       Start  5: test_wrap_cpp_options
#  5/11 Test  #5: test_wrap_cpp_options ............   Passed    0.36 sec
#       Start  6: test_needsquoting_dirname
#  6/11 Test  #6: test_needsquoting_dirname ........   Passed    2.20 sec
#       Start  7: test_platform_defs_include
#  7/11 Test  #7: test_platform_defs_include .......   Passed    0.28 sec
#       Start  8: test_qtmainwin_library
#  8/11 Test  #8: test_qtmainwin_library ...........   Passed    1.27 sec
#       Start  9: test_dbus_module
#  9/11 Test  #9: test_dbus_module .................   Passed    3.46 sec
#       Start 10: test_multiple_find_package
# 10/11 Test #10: test_multiple_find_package .......   Passed    0.07 sec
#       Start 11: test_add_resources_delayed_file
# 11/11 Test #11: test_add_resources_delayed_file ..   Passed    0.38 sec
#
#
# Note that if Qt is not installed, or if it is installed to a
# non-standard prefix, the environment variable CMAKE_PREFIX_PATH
# needs to be set to the installation prefix or build prefix of Qt
# before running these tests.

cmake_minimum_required(VERSION 3.16)

project(cmake_usage_tests)
include(GNUInstallDirs)

# Building the CMake tests as part of a Qt prefix build + in-tree tests, currently doesn't work.
# Each CMake test will fail with a message like
#
# CMake Error at qtbase/lib/cmake/Qt6/Qt6Config.cmake:33 (include):
#   include could not find load file:
#    qtbase/lib/cmake/Qt6/Qt6Targets.cmake
#
# That's because the Qt packages are not installed, and we try to load the Config files from the
# build dir, but they can't work in a prefix build without installation.
# Configuring the tests as standalone tests or as a separate project works fine.
# Configuring the tests in-tree also works fine in a non-prefix build.
if(QT_REPO_MODULE_VERSION AND NOT QT_BUILD_STANDALONE_TESTS AND QT_WILL_INSTALL)
    message(WARNING
        "Skipping building CMake build tests because they don't work in a prefix in-tree config")
endif()

enable_testing()

# Most of the tests fail to build on Boot2qt / qemu with undefined references to QtDBus because
# it's a private dependency of QtGui, and CMake for some reason doesn't generate an -rpath-link
# flag. Notably -rpath is specified which should implicitly enable -rpath-link, but that
# doesn't seem to be the case.
# Until this is figured out, disable the tests when cross-compiling to Linux.
if(UNIX AND NOT APPLE AND NOT WIN32 AND CMAKE_CROSSCOMPILING AND NOT QT_ENABLE_CMAKE_BOOT2QT_TESTS
    AND NOT QT_BUILD_MINIMAL_ANDROID_MULTI_ABI_TESTS)
    message(STATUS "Running CMake tests is disabled when cross-compiling to Linux / Boot2Qt.")
    return()
endif()

set(required_packages Core Network Xml Sql Test)
set(optional_packages DBus Gui Widgets PrintSupport OpenGL Concurrent)

# Setup the test when called as a completely standalone project.
if(TARGET Qt6::Core)
    # Tests are built as part of the qtbase build tree.
    # Setup paths so that the Qt packages are found, similar to examples.
    qt_internal_set_up_build_dir_package_paths()
endif()
find_package(Qt6 REQUIRED COMPONENTS ${required_packages})
find_package(Qt6 OPTIONAL_COMPONENTS ${optional_packages})

# Setup common test variables which were previously set by ctest_testcase_common.prf.
set(CMAKE_MODULES_UNDER_TEST "${required_packages}" ${optional_packages})

foreach(qt_package ${CMAKE_MODULES_UNDER_TEST})
    set(package_name "${QT_CMAKE_EXPORT_NAMESPACE}${qt_package}")
    if(${package_name}_FOUND)
        set(CMAKE_${qt_package}_MODULE_MAJOR_VERSION "${${package_name}_VERSION_MAJOR}")
        set(CMAKE_${qt_package}_MODULE_MINOR_VERSION "${${package_name}_VERSION_MINOR}")
        set(CMAKE_${qt_package}_MODULE_PATCH_VERSION "${${package_name}_VERSION_PATCH}")
    endif()
endforeach()

# Qt6CTestMacros.cmake also expects some of these variables to be set.
if(NOT TARGET Qt::Gui)
    set(NO_GUI TRUE)
endif()
if(NOT TARGET Qt::DBus)
    set(NO_DBUS TRUE)
endif()
if(NOT TARGET Qt::Widgets)
    set(NO_WIDGETS TRUE)
endif()

include("${_Qt6CTestMacros}")

# Test only multi-abi specific functionality when QT_BUILD_MINIMAL_ANDROID_MULTI_ABI_TESTS is ON.
# Qt::Gui is the prerequisite for all Android tests.
if(QT_BUILD_MINIMAL_ANDROID_MULTI_ABI_TESTS AND NOT NO_GUI)
    unset(multi_abi_vars)
    foreach(abi IN LISTS QT_ANDROID_ABIS)
        list(APPEND multi_abi_vars "-DQT_PATH_ANDROID_ABI_${abi}=${QT_PATH_ANDROID_ABI_${abi}}")
    endforeach()
    if(QT_ANDROID_BUILD_ALL_ABIS)
        list(APPEND multi_abi_vars "-DQT_ANDROID_BUILD_ALL_ABIS=${QT_ANDROID_BUILD_ALL_ABIS}")
    endif()

    list(APPEND multi_abi_vars "-DQT_HOST_PATH=${QT_HOST_PATH}")

    set(multi_abi_forward_vars
        TEST_SINGLE_VALUE_ARG
        TEST_SPACES_VALUE_ARG
        TEST_LIST_VALUE_ARG
        TEST_ESCAPING_VALUE_ARG
    )
    string(REPLACE ";" "[[;]]" multi_abi_forward_vars "${multi_abi_forward_vars}")

    set(single_value "TestValue")
    set(list_value "TestValue[[;]]TestValue2[[;]]TestValue3")
    set(escaping_value "TestValue\\\\[[;]]TestValue2\\\\[[;]]TestValue3")
    set(spaces_value "TestValue TestValue2 TestValue3")
    _qt_internal_test_expect_pass(test_android_multi_abi_forward_vars
        BUILD_OPTIONS
            ${multi_abi_vars}
            "-DQT_ANDROID_MULTI_ABI_FORWARD_VARS=${multi_abi_forward_vars}"
            "-DTEST_SINGLE_VALUE_ARG=${single_value}"
            "-DTEST_LIST_VALUE_ARG=${list_value}"
            "-DTEST_ESCAPING_VALUE_ARG=${escaping_value}"
            "-DTEST_SPACES_VALUE_ARG=${spaces_value}"
    )
    return()
endif()

if(NOT NO_WIDGETS)
    _qt_internal_test_expect_pass(test_build_simple_widget_app)
    set(extra_widget_app_options "")
    if(IOS)
        list(APPEND extra_widget_app_options
            QMAKE_OPTIONS CONFIG+=iossimulator
        )
    endif()
    if(CMAKE_HOST_WIN32)
        # Unset MAKEFLAGS environment variable when invoking build tool, it might
        # have options incompatible with nmake.
        list(APPEND extra_widget_app_options
            BUILD_ENVIRONMENT MAKEFLAGS ""
        )
    endif()

    _qt_internal_add_qmake_test(test_build_simple_widget_app
        TESTNAME test_build_simple_widget_app_qmake
        ${extra_widget_app_options}
    )
endif()

# We only support a limited subset of cmake tests when targeting iOS:
# - Only those that use qt_add_executable (but not add_executable)
# - and don't try to run the built binaries via BINARY_ARGS option
# - and don't use internal API like qt_internal_add_*
#
# So we can't run binaries in the simulator or on-device, but we at least
# want build coverage (app linking succeeds).
if(IOS)
    return()
endif()

set(is_qt_build_platform TRUE)
# macOS versions less than 10.15 are not supported for building Qt.
if(CMAKE_HOST_APPLE AND CMAKE_HOST_SYSTEM_VERSION VERSION_LESS "19.0.0")
    set(is_qt_build_platform FALSE)
endif()

_qt_internal_test_expect_pass(test_umbrella_config)
_qt_internal_test_expect_pass(test_wrap_cpp_and_resources)
if (NOT NO_WIDGETS)
    _qt_internal_test_expect_pass(test_dependent_modules)
    _qt_internal_test_expect_pass("test(needsquoting)dirname")
endif()
_qt_internal_test_expect_pass(test_add_resource_prefix BINARY test_add_resource_prefix)
_qt_internal_test_expect_build_fail(test_add_resource_options)
_qt_internal_test_expect_build_fail(test_wrap_cpp_options)
_qt_internal_test_expect_pass(test_platform_defs_include)
_qt_internal_test_expect_pass(test_qtmainwin_library)

if (CMAKE_GENERATOR STREQUAL Ninja AND UNIX AND NOT WIN32)
    _qt_internal_test_expect_pass(test_QFINDTESTDATA
        BINARY "tests/test_QFINDTESTDATA"
        SIMULATE_IN_SOURCE
    )
    # TODO: Decide if there's a reason to keep this test. With CMake 3.21.0 which passes absolute
    # source file paths to the compiler (instead of relative ones), specifying a custom
    # QT_TESTCASE_BUILDDIR is a no-op, which fails the test's preconditions.
    # See QTBUG-95268.
    #_qt_internal_test_expect_pass(test_QT_TESTCASE_BUILDDIR
    #    BINARY "test_qt_testcase_builddir"
    #    SIMULATE_IN_SOURCE
    #)
endif()

if (NOT NO_DBUS)
    _qt_internal_test_expect_pass(test_dbus_module)
endif()
_qt_internal_test_expect_pass(test_multiple_find_package)
_qt_internal_test_expect_pass(test_add_resources_delayed_file)
_qt_internal_test_expect_pass(test_add_binary_resources_delayed_file BINARY test_add_binary_resources_delayed_file)
_qt_internal_test_expect_pass(test_qt_add_resources_rebuild)

if(NOT NO_GUI)
    _qt_internal_test_expect_pass(test_private_includes)
    _qt_internal_test_expect_pass(test_private_targets)
endif()

_qt_internal_test_expect_pass(test_testlib_definitions)
_qt_internal_test_expect_pass(test_json_plugin_includes)

if(NOT NO_GUI)
    _qt_internal_test_expect_build_fail(test_testlib_no_link_gui)
    execute_process(COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/test_testlib_definitions/main.cpp"
        "${CMAKE_CURRENT_BINARY_DIR}/failbuild/test_testlib_no_link_gui/test_testlib_no_link_gui/"
    )
endif()

if (NOT NO_WIDGETS)
    _qt_internal_test_expect_build_fail(test_testlib_no_link_widgets)
    execute_process(COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/test_testlib_definitions/main.cpp"
        "${CMAKE_CURRENT_BINARY_DIR}/failbuild/test_testlib_no_link_widgets/test_testlib_no_link_widgets/"
    )
endif()

set(qt_module_includes
  Core QObject
  Network QHostInfo
  Sql QSqlError
  Test QTestEventList
  Xml QDomDocument
)

if (NOT NO_GUI)
  list(APPEND qt_module_includes
    Gui QImage
  )
endif()

if (NOT NO_WIDGETS)
  list(APPEND qt_module_includes
    Widgets QWidget
    OpenGL QOpenGLBuffer
    PrintSupport QPrinter
  )
endif()

if (NOT NO_DBUS)
  list(APPEND qt_module_includes
    DBus QDBusMessage
  )
endif()

_qt_internal_test_module_includes(
  ${qt_module_includes}
)
_qt_internal_test_expect_pass(test_concurrent_module)

if(NOT NO_GUI)
    _qt_internal_test_expect_pass(test_opengl_lib)
endif()

if (NOT NO_WIDGETS)
    _qt_internal_test_expect_pass(test_interface)
endif()

if(NOT NO_GUI)
    _qt_internal_test_expect_pass(test_interface_link_libraries)
endif()
_qt_internal_test_expect_pass(test_moc_macro_target)

# The modification of TARGET_OBJECTS needs the following change in cmake
# https://gitlab.kitware.com/cmake/cmake/commit/93c89bc75ceee599ba7c08b8fe1ac5104942054f
_qt_internal_test_expect_pass(test_add_big_resource)

# With earlier CMake versions, this test would simply run moc multiple times and lead to:
# /usr/bin/ld: error: CMakeFiles/mywidget.dir/mywidget_automoc.cpp.o: multiple definition of 'MyWidget::qt_static_metacall(QObject*, QMetaObject::Call, int, void**)'
# /usr/bin/ld: CMakeFiles/mywidget.dir/moc_mywidget.cpp.o: previous definition here
# Reason: SKIP_* properties were added in CMake 3.8 only
if(NOT NO_WIDGETS)
    _qt_internal_test_expect_pass(test_QTBUG-63422)
endif()

# Find main Qt installation location and bin dir.
if(QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX)
    set(qt_install_prefix "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}")
elseif(QT6_INSTALL_PREFIX)
    set(qt_install_prefix "${QT6_INSTALL_PREFIX}")
endif()

if(INSTALL_LIBEXECDIR)
    set(qt_install_libexec_dir "${INSTALL_LIBEXECDIR}")
elseif(QT6_INSTALL_LIBEXECS)
    set(qt_install_libexec_dir "${QT6_INSTALL_LIBEXECS}")
endif()

# Test building and installing a few dummy Qt modules and plugins.
if(is_qt_build_platform)
    set(mockplugins_test_args "")
     if(NOT QT_FEATURE_no_prefix)
        list(APPEND mockplugins_test_args
            BINARY "${CMAKE_COMMAND}"
            BINARY_ARGS
            "-DQT_BUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}/mockplugins"
            -P "${qt_install_prefix}/${qt_install_libexec_dir}/qt-cmake-private-install.cmake"
        )
    endif()
    _qt_internal_test_expect_pass(mockplugins ${mockplugins_test_args})
    set_tests_properties(mockplugins PROPERTIES FIXTURES_SETUP build_mockplugins)

    # Test importing the plugins built in the project above.
    _qt_internal_test_expect_pass(test_import_plugins BINARY ${CMAKE_CTEST_COMMAND} BINARY_ARGS -V)
    set_tests_properties(test_import_plugins PROPERTIES FIXTURES_REQUIRED build_mockplugins)
endif()

_qt_internal_test_expect_pass(test_versionless_targets)

if(NOT NO_GUI)
  _qt_internal_test_expect_pass(test_global_promotion)
endif()

_qt_internal_test_expect_pass(test_add_resources_binary_generated
                              BINARY test_add_resources_binary_generated)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.17")
    _qt_internal_test_expect_pass(test_add_resources_big_resources
        BINARY test_add_resources_big_resources)
endif()

include(test_plugin_shared_static_flavor.cmake)
_qt_internal_test_expect_pass(tst_qaddpreroutine
                              BINARY tst_qaddpreroutine)

if(is_qt_build_platform)
    _qt_internal_test_expect_pass(test_static_resources
                                BINARY "${CMAKE_CTEST_COMMAND}"
                                BINARY_ARGS "-V")

    _qt_internal_test_expect_pass(test_generating_cpp_exports)
endif()

_qt_internal_test_expect_pass(test_qt_extract_metatypes)

set(deploy_args
    test_widgets_app_deployment
    BINARY "${CMAKE_CTEST_COMMAND}"
    BINARY_ARGS "-V"
    # Need to explicitly specify a writable install prefix.
    BUILD_OPTIONS
        -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/test_widgets_app_deployment_installed
    NO_RUN_ENVIRONMENT_PLUGIN_PATH
)

set(is_desktop_linux FALSE)
if(UNIX AND NOT APPLE AND NOT ANDROID AND NOT CMAKE_CROSSCOMPILING)
    set(is_desktop_linux TRUE)
endif()

# For now, the test should only pass on Windows, macOS and desktop Linux shared and static builds
# and fail on other platforms, because there is no support for runtime dependency deployment on
# those platforms.
# With static builds the runtime dependencies are just skipped, but the test should still pass.
if(WIN32 OR (APPLE AND NOT IOS) OR is_desktop_linux)
    _qt_internal_test_expect_pass(${deploy_args})
else()
    _qt_internal_test_expect_fail(${deploy_args})
endif()
