# Generated from qcryptographichash.pro.

#####################################################################
## tst_qcryptographichash Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qcryptographichash
    SOURCES
        tst_qcryptographichash.cpp
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

if((ANDROID AND NOT ANDROID_EMBEDDED) OR INTEGRITY)
    # Resources:
    set(testdata_resource_files
        "data/2c1517dad3678f03917f15849b052fd5.md5"
        "data/d41d8cd98f00b204e9800998ecf8427e.md5"
    )

    qt_internal_add_resource(tst_qcryptographichash "testdata"
        PREFIX
            "/"
        FILES
            ${testdata_resource_files}
    )
endif()
