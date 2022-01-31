# Generated from qstringconverter.pro.

#####################################################################
## tst_qstringconverter Test:
#####################################################################

qt_internal_add_test(tst_qstringconverter
    SOURCES
        tst_qstringconverter.cpp
    TESTDATA ${test_data}
    PUBLIC_LIBRARIES
        Qt::CorePrivate # for access to Qt's feature system
)


qt_internal_add_resource(tst_qstringconverter  "compressedtexture_bc1"
    PREFIX
        "/"
    FILES
        "euc_kr.txt"
)
