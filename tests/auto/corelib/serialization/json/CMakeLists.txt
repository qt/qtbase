# Generated from json.pro.

#####################################################################
## tst_json Test:
#####################################################################

qt_internal_add_test(tst_json
    SOURCES
        tst_qtjson.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
)

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:json.pro:NOT ANDROID:
# TESTDATA = "bom.json" "test.json" "test3.json" "test2.json"

if(ANDROID OR INTEGRITY)
    # Resources:
    set(json_resource_files
        "bom.json"
        "test.json"
        "test2.json"
        "test3.json"
    )

    qt_internal_add_resource(tst_json "json"
        PREFIX
            "/"
        FILES
            ${json_resource_files}
    )
endif()

qt_internal_extend_target(tst_json CONDITION NOT QT_FEATURE_doubleconversion AND NOT QT_FEATURE_system_doubleconversion
    DEFINES
        QT_NO_DOUBLECONVERSION
)
