# Generated from qzip.pro.

#####################################################################
## tst_qzip Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    testdata/*
)

qt_internal_add_test(tst_qzip
    SOURCES
        tst_qzip.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
    TESTDATA ${test_data}
)
