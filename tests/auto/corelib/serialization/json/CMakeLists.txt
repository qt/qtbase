# Generated from json.pro.

#####################################################################
## tst_json Test:
#####################################################################

set(json_resource_files
    "bom.json"
    "test.json"
    "test2.json"
    "test3.json"
    "simple.duplicates.json"
    "test.duplicates.json"
    "test3.duplicates.json"
)

qt_internal_add_test(tst_json
    SOURCES
        tst_qtjson.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
    TESTDATA ${json_resource_files}
)

qt_internal_extend_target(tst_json CONDITION NOT QT_FEATURE_doubleconversion AND NOT QT_FEATURE_system_doubleconversion
    DEFINES
        QT_NO_DOUBLECONVERSION
)
