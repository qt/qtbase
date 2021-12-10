# Generated from qchar.pro.

#####################################################################
## tst_qchar Test:
#####################################################################

# Collect test data
list(APPEND test_data "data/NormalizationTest.txt")

qt_internal_add_test(tst_qchar
    SOURCES
        tst_qchar.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

if((ANDROID AND NOT ANDROID_EMBEDDED) OR INTEGRITY)
    # Resources:
    set(testdata_resource_files
        "data/NormalizationTest.txt"
    )

    qt_internal_add_resource(tst_qchar "testdata"
        PREFIX
            "/"
        FILES
            ${testdata_resource_files}
    )
endif()
