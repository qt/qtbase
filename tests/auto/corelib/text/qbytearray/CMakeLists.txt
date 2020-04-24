# Generated from qbytearray.pro.

#####################################################################
## tst_qbytearray Test:
#####################################################################

# Collect test data
list(APPEND test_data "rfc3252.txt")

qt_add_test(tst_qbytearray
    SOURCES
        tst_qbytearray.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

qt_extend_target(tst_qbytearray CONDITION APPLE
    SOURCES
        tst_qbytearray_mac.mm
    PUBLIC_LIBRARIES
        ${FWFoundation}
)

if(ANDROID AND NOT ANDROID_EMBEDDED)
    # Resources:
    set(android_testdata_resource_files
        "rfc3252.txt"
    )

    qt_add_resource(tst_qbytearray "android_testdata"
        PREFIX
            "/"
        FILES
            ${android_testdata_resource_files}
    )
endif()
