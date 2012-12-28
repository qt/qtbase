
cmake_minimum_required(VERSION 2.8)

project(no_link_gui)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/FindPackageHints.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/FindPackageHints.cmake")
endif()

find_package(Qt5Gui REQUIRED HINTS ${Qt5Tests_PREFIX_PATH})
find_package(Qt5Test REQUIRED HINTS ${Qt5Tests_PREFIX_PATH})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Core_EXECUTABLE_COMPILE_FLAGS}")

include_directories(
    ${Qt5Gui_INCLUDE_DIRS}
    ${Qt5Test_INCLUDE_DIRS}
)

add_definitions(
    ${Qt5Gui_DEFINITIONS}
    ${Qt5Test_DEFINITIONS}
)

set(main_file "main.cpp")
set(moc_file "${CMAKE_CURRENT_BINARY_DIR}/main.moc")

qt5_generate_moc("${main_file}" "${moc_file}")

# The core_test is expected to fail to build because
# QT_GUI_LIB is defined, which affects the contents of
# Qt Test and the definition of QTEST_MAIN.
# If running this test manually (ctest -V -R no_link_gui from
# the tests/auto/cmake/build directory), the core_test is
# expected to fail to link because of missing symbols from Qt GUI.
# The gui_test is expected to build successfully  (though it may
# be necessary to comment out the core_test and re-run cmake)
add_executable(core_test "${main_file}" "${moc_file}")
target_link_libraries(core_test
    ${Qt5Core_LIBRARIES}
    ${Qt5Test_LIBRARIES}
)

add_executable(gui_test "${main_file}" "${moc_file}")
target_link_libraries(gui_test
    ${Qt5Gui_LIBRARIES}
    ${Qt5Test_LIBRARIES}
)
