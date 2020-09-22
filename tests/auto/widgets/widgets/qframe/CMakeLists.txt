# Generated from qframe.pro.

#####################################################################
## tst_qframe Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    images/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qframe
    LOWDPI # special case
    SOURCES
        tst_qframe.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
    TESTDATA ${test_data}
)
