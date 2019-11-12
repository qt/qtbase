# Generated from json.pro.

#####################################################################
## tst_json Test:
#####################################################################

add_qt_test(tst_json
    SOURCES
        tst_qtjson.cpp
    LIBRARIES
        Qt::CorePrivate
)

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:json.pro:NOT ANDROID:
# TESTDATA = "bom.json" "test.json" "test.bjson" "test3.json" "test2.json"

if(ANDROID)
    # Resources:
    set(json_resource_files
        "bom.json"
        "test.bjson"
        "test.json"
        "test2.json"
        "test3.json"
    )

    add_qt_resource(tst_json "json"
        PREFIX
            "/"
        FILES
            ${json_resource_files}
    )
endif()
