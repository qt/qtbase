# Generated from qloggingregistry.pro.

#####################################################################
## tst_qloggingregistry Test:
#####################################################################

# Collect test data
list(APPEND test_data "qtlogging.ini")

qt_internal_add_test(tst_qloggingregistry
    SOURCES
        tst_qloggingregistry.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qloggingregistry.pro:<TRUE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################

if(ANDROID AND NOT ANDROID_EMBEDDED)
    # Resources:
    set(android_testdata_resource_files
        "qtlogging.ini"
    )

    qt_internal_add_resource(tst_qloggingregistry "android_testdata"
        PREFIX
            "/"
        FILES
            ${android_testdata_resource_files}
    )
endif()
