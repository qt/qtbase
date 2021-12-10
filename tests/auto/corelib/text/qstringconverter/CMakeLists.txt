# Generated from qstringconverter.pro.

#####################################################################
## tst_qstringconverter Test:
#####################################################################

list(APPEND test_data "utf8.txt")

qt_internal_add_test(tst_qstringconverter
    SOURCES
        tst_qstringconverter.cpp
    TESTDATA ${test_data}
)

if(INTEGRITY)
    # Resources:
    set(testdata_resource_files
        "utf8.txt"
    )

    qt_internal_add_resource(tst_qstringconverter "testdata"
        PREFIX
            "/"
        FILES
            ${testdata_resource_files}
    )
endif()
