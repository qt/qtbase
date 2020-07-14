
add_executable(test_QFINDTESTDATA WIN32 main.cpp)

target_link_libraries(test_QFINDTESTDATA Qt::Test)
target_compile_definitions(test_QFINDTESTDATA PRIVATE QT_TESTCASE_BUILDDIR="${CMAKE_CURRENT_BINARY_DIR}")
target_compile_definitions(test_QFINDTESTDATA PRIVATE QT_TESTCASE_SOURCEDIR="${CMAKE_CURRENT_SOURCE_DIR}")
