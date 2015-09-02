TARGET = tst_json
QT = core testlib
CONFIG -= app_bundle
CONFIG += testcase

!android:TESTDATA += test.json test.bjson test3.json test2.json
    else:RESOURCES += json.qrc

SOURCES += tst_qtjson.cpp
