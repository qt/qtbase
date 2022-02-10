# Generated from qdatastream.pro.

#####################################################################
## tst_qdatastream Test:
#####################################################################

# Collect test data
list(APPEND test_data "datastream.q42")
list(APPEND test_data "typedef.q5")

qt_internal_add_test(tst_qdatastream
    SOURCES
        tst_qdatastream.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
    TESTDATA ${test_data}
)
