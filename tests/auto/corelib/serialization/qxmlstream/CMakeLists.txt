# Generated from qxmlstream.pro.

#####################################################################
## tst_qxmlstream Test:
#####################################################################

# Collect test data
list(APPEND test_data "data")
list(APPEND test_data "XML-Test-Suite")

qt_internal_add_test(tst_qxmlstream
    SOURCES
        tst_qxmlstream.cpp
    LIBRARIES
        Qt::Network
        Qt::Xml
        Qt::GuiPrivate
    TESTDATA ${test_data}
)
