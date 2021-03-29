# special case skip regeneration

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

cmake_minimum_required(VERSION 3.14)

project(cmake_usage_tests)

enable_testing()

# Most of the tests fail to build on Boot2qt / qemu with undefined references to QtDBus because
# it's a private dependency of QtGui, and CMake for some reason doesn't generate an -rpath-link
# flag. Notably -rpath is specified which should implicitly enable -rpath-link, but that
# doesn't seem to be the case.
# Until this is figured out, disable the tests when cross-compiling to Linux.
if(UNIX AND NOT APPLE AND NOT WIN32 AND CMAKE_CROSSCOMPILING AND NOT QT_ENABLE_CMAKE_BOOT2QT_TESTS)
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
set(CMAKE_MODULES_UNDER_TEST "${required_packages}")
foreach(qt_package ${optional_packages})
    set(package_name "${QT_CMAKE_EXPORT_NAMESPACE}${qt_package}")
    list(APPEND CMAKE_MODULES_UNDER_TEST "${qt_package}")
endforeach()

foreach(qt_package ${CMAKE_MODULES_UNDER_TEST})
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

_qt_internal_test_expect_pass(test_umbrella_config)
_qt_internal_test_expect_pass(test_wrap_cpp_and_resources)
if (NOT NO_WIDGETS)
    _qt_internal_test_expect_pass(test_dependent_modules)
    _qt_internal_test_expect_pass("test(needsquoting)dirname")
endif()
_qt_internal_test_expect_build_fail(test_add_resource_options)
_qt_internal_test_expect_build_fail(test_wrap_cpp_options)
_qt_internal_test_expect_pass(test_platform_defs_include)
_qt_internal_test_expect_pass(test_qtmainwin_library)

if (CMAKE_GENERATOR STREQUAL Ninja AND UNIX AND NOT WIN32)
    _qt_internal_test_expect_pass(test_QFINDTESTDATA
        BINARY "tests/test_QFINDTESTDATA"
        SIMULATE_IN_SOURCE
    )
    _qt_internal_test_expect_pass(test_QT_TESTCASE_BUILDDIR
        BINARY "test_qt_testcase_builddir"
        SIMULATE_IN_SOURCE
    )
endif()

if (NOT NO_DBUS)
    _qt_internal_test_expect_pass(test_dbus_module)
endif()
_qt_internal_test_expect_pass(test_multiple_find_package)
_qt_internal_test_expect_pass(test_add_resources_delayed_file)
_qt_internal_test_expect_pass(test_add_binary_resources_delayed_file BINARY test_add_binary_resources_delayed_file)

if(NOT NO_GUI)
    _qt_internal_test_expect_pass(test_private_includes)
    _qt_internal_test_expect_pass(test_private_targets)
endif()

_qt_internal_test_expect_pass(test_testlib_definitions)
_qt_internal_test_expect_pass(test_json_plugin_includes)

if(NOT NO_GUI)
    _qt_internal_test_expect_build_fail(test_testlib_no_link_gui)
endif()

execute_process(COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/test_testlib_definitions/main.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/failbuild/test_testlib_no_link_gui/test_testlib_no_link_gui/"
)

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

# FIXME: Needs porting of the qmake .pro files to create the modules and plugins in Qt6 CMake land.
# _qt_internal_test_expect_pass(test_import_plugins BINARY ${CMAKE_CTEST_COMMAND})
_qt_internal_test_expect_pass(test_versionless_targets)

_qt_internal_test_expect_pass(test_add_resources_binary_generated
                              BINARY test_add_resources_binary_generated)

include(test_plugin_shared_static_flavor.cmake)
_qt_internal_test_expect_pass(test_init_resources_static_plugin
                              BINARY test_init_resources_static_plugin)
