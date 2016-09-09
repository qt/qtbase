TARGET = tst_json
QT = core-private testlib
CONFIG -= app_bundle
CONFIG += testcase

!android:TESTDATA += bom.json test.json test.bjson test3.json test2.json
    else:RESOURCES += json.qrc

!qtConfig(doubleconversion):!qtConfig(system-doubleconversion) {
    DEFINES += QT_NO_DOUBLECONVERSION
}

SOURCES += tst_qtjson.cpp
