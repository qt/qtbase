# Generated from qlayout.pro.

#####################################################################
## tst_qlayout Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    baseline/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qlayout
    SOURCES
        tst_qlayout.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::TestPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
    TESTDATA ${test_data}
)
