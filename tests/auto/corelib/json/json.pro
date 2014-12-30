TARGET = tst_json
QT = core testlib
CONFIG -= app_bundle
CONFIG += testcase
CONFIG += parallel_test

!android:TESTDATA += test.json test.bjson test3.json test2.json
    else:RESOURCES += json.qrc

SOURCES += tst_qtjson.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
