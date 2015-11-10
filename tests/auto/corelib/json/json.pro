TARGET = tst_json
QT = core testlib
CONFIG -= app_bundle
CONFIG += testcase

!android:TESTDATA += bom.json test.json test.bjson test3.json test2.json
    else:RESOURCES += json.qrc

!contains(QT_CONFIG, doubleconversion):!contains(QT_CONFIG, system-doubleconversion) {
    DEFINES += QT_NO_DOUBLECONVERSION
}

SOURCES += tst_qtjson.cpp
