
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

expect_pass(test_use_modules_function)
expect_pass(test_umbrella_config)
expect_pass(test_wrap_cpp_and_resources)
if (NOT NO_WIDGETS)
    expect_pass(test_dependent_modules)
    expect_pass("test(needsquoting)dirname")
endif()
expect_fail(test_add_resource_options)
expect_fail(test_wrap_cpp_options)
expect_pass(test_platform_defs_include)
expect_pass(test_qtmainwin_library)

if (HAVE_NINJA)
    make_directory("${CMAKE_CURRENT_SOURCE_DIR}/test_QFINDTESTDATA/build")
    add_test(test_QFINDTESTDATA ${CMAKE_CTEST_COMMAND}
      --build-and-test
      "${CMAKE_CURRENT_SOURCE_DIR}/test_QFINDTESTDATA"
      # Build in a subdir of the source dir.
      # This causes Ninja to use relative paths.
      "${CMAKE_CURRENT_SOURCE_DIR}/test_QFINDTESTDATA/build"
      --build-generator Ninja
      --build-options "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}"
    )
    add_test(NAME run_test_QFINDTESTDATA COMMAND sh -c "cd \"${CMAKE_SOURCE_DIR}/test_QFINDTESTDATA/build/tests\" && ./test_QFINDTESTDATA -v2")

    set_property(TEST run_test_QFINDTESTDATA
        PROPERTY DEPENDS test_QFINDTESTDATA
    )
endif()

if (NOT NO_DBUS)
    expect_pass(test_dbus_module)
endif()
expect_pass(test_multiple_find_package)
expect_pass(test_add_resources_delayed_file)
expect_pass(test_add_binary_resources_delayed_file BINARY test_add_binary_resources_delayed_file)
expect_pass(test_private_includes)
expect_pass(test_private_targets)
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

if (NOT NO_WIDGETS)
    expect_pass(test_interface)
endif()

expect_pass(test_interface_link_libraries)
expect_pass(test_moc_macro_target)

if (NOT CMAKE_VERSION VERSION_LESS 3.9)
    # The modification of TARGET_OBJECTS needs the following change in cmake
    # https://gitlab.kitware.com/cmake/cmake/commit/93c89bc75ceee599ba7c08b8fe1ac5104942054f
    expect_pass(test_add_big_resource)
endif()

if (NOT CMAKE_VERSION VERSION_LESS 3.8)
    # With earlier CMake versions, this test would simply run moc multiple times and lead to:
    # /usr/bin/ld: error: CMakeFiles/mywidget.dir/mywidget_automoc.cpp.o: multiple definition of 'MyWidget::qt_static_metacall(QObject*, QMetaObject::Call, int, void**)'
    # /usr/bin/ld: CMakeFiles/mywidget.dir/moc_mywidget.cpp.o: previous definition here
    # Reason: SKIP_* properties were added in CMake 3.8 only
    expect_pass(test_QTBUG-63422)
endif()

expect_pass(test_import_plugins BINARY ${CMAKE_CTEST_COMMAND})
expect_pass(test_versionless_targets)
