# Generated from qdom.pro.

#####################################################################
## tst_qdom Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    testdata/*)
list(APPEND test_data ${test_data_glob})
list(APPEND test_data "doubleNamespaces.xml")
list(APPEND test_data "umlaut.xml")

qt_internal_add_test(tst_qdom
    SOURCES
        tst_qdom.cpp
    PUBLIC_LIBRARIES
        Qt::Xml
    TESTDATA ${test_data}
)
