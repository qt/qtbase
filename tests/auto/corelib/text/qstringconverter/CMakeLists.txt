# Generated from qstringconverter.pro.

#####################################################################
## tst_qstringconverter Test:
#####################################################################

qt_internal_add_test(tst_qstringconverter
    SOURCES
        tst_qstringconverter.cpp
    TESTDATA ${test_data}
)

if(INTEGRITY)
    qt_internal_add_resource(tst_qstringconverter "testdata"
        PREFIX
            "/"
        FILES
            ${testdata_resource_files}
    )
endif()
