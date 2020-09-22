# Generated from qplugin.pro.

add_subdirectory(invalidplugin)

# special case begin
add_subdirectory(debugplugin)
add_subdirectory(releaseplugin)

# The contents below are generated from ./tst_qplugin.pro
# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    plugins/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qplugin
    SOURCES
        tst_qplugin.cpp
    LIBRARIES
        Qt::CorePrivate
    TESTDATA ${test_data}
)

target_compile_definitions(tst_qplugin PRIVATE CMAKE_BUILD=1)

add_dependencies(tst_qplugin invalidplugin debugplugin releaseplugin)
# special case end
