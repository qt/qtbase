CONFIG += testcase
TARGET = tst_qfontdatabase
SOURCES  += tst_qfontdatabase.cpp
QT += testlib core-private gui-private
TESTDATA += LED_REAL.TTF

android {
    RESOURCES += testdata.qrc
}

