# Generated from qmetacontainer.pro.

#####################################################################
## tst_qmetacontainer Test:
#####################################################################

# Collect test data
list(APPEND test_data "./typeFlags.bin")

qt_add_test(tst_qmetacontainer
    SOURCES
        tst_qmetacontainer.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
    TESTDATA ${test_data}
)
