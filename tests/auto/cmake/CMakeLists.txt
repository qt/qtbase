
# This is an automatic test for the CMake configuration files.
# To run it manually,
# 1) mkdir build   # Create a build directory
# 2) cd build
# 3) cmake ..      # Run cmake on this directory.
# 4) ctest         # Run ctest
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

cmake_minimum_required(VERSION 2.8)

project(qmake_cmake_files)

enable_testing()

find_package(Qt5Core REQUIRED)

include("${_Qt5CTestMacros}")

if(NOT ${CMAKE_VERSION} VERSION_LESS 2.8.9)
    # Requires INCLUDE_DIRECTORIES target property in CMake 2.8.8
    # and POSITION_INDEPENDENT_CODE target property in 2.8.9
    expect_pass(test_use_modules_function)
    expect_pass(test_umbrella_config)
else()
    message("CMake version older than 2.8.9 (Found ${CMAKE_VERSION}). Not running test \"test_use_modules_function\" or \"test_umbrella_config\"")
endif()

expect_pass(test_wrap_cpp_and_resources)
if (NOT NO_WIDGETS)
    expect_pass(test_dependent_modules)
    expect_pass("test(needsquoting)dirname")
endif()
expect_fail(test_add_resource_options)
expect_fail(test_wrap_cpp_options)
expect_pass(test_platform_defs_include)
expect_pass(test_qtmainwin_library)

if (NOT NO_DBUS)
    expect_pass(test_dbus_module)
endif()
expect_pass(test_multiple_find_package)
if (NOT WIN32 OR (WIN32 AND NOT CMAKE_VERSION VERSION_LESS 2.8.11))
    # Broken on windows on earlier CMake versions.
    # http://public.kitware.com/Bug/view.php?id=13392
    expect_pass(test_add_resources_delayed_file)
endif()
expect_pass(test_private_includes)
expect_pass(test_testlib_definitions)
expect_pass(test_json_plugin_includes)

expect_fail(test_testlib_no_link_gui)
execute_process(COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/test_testlib_definitions/main.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/failbuild/test_testlib_no_link_gui/test_testlib_no_link_gui/"
)

if (NOT NO_WIDGETS)
    expect_fail(test_testlib_no_link_widgets)
    execute_process(COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/test_testlib_definitions/main.cpp"
        "${CMAKE_CURRENT_BINARY_DIR}/failbuild/test_testlib_no_link_widgets/test_testlib_no_link_widgets/"
    )
endif()

set(qt_module_includes
  Core QObject
  Gui QImage
  Network QHostInfo
  Sql QSqlError
  Test QTestEventList
  Xml QDomDocument
)

if (NOT NO_WIDGETS)
  list(APPEND qt_module_includes
    Widgets QWidget
    OpenGL QGLBuffer
    PrintSupport QPrinter
  )
endif()

if (NOT NO_DBUS)
  list(APPEND qt_module_includes
    DBus QDBusMessage
  )
endif()

test_module_includes(
  ${qt_module_includes}
)
expect_pass(test_concurrent_module)
expect_pass(test_openglextensions_module)
if (QT_WITH_ANGLE OR (NOT WIN32 AND NOT APPLE AND NOT NO_EGL))
    expect_pass(test_egl_lib)
endif()
expect_pass(test_opengl_lib)

if (NOT CMAKE_VERSION VERSION_LESS 2.8.11)
    expect_pass(test_interface)
endif()
