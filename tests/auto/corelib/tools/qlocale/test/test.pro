CONFIG += testcase
QT = core testlib network
embedded: QT += gui
SOURCES = ../tst_qlocale.cpp

TARGET = ../tst_qlocale
win32 {
    CONFIG(debug, debug|release) {
        TARGET = ../../debug/tst_qlocale
    } else {
        TARGET = ../../release/tst_qlocale
    }
}
TESTDATA += syslocaleapp

mac: CONFIG += insignificant_test # QTBUG-22769
