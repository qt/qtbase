CONFIG += testcase
TARGET  = ../tst_qpluginloader
QT = core testlib
SOURCES = ../tst_qpluginloader.cpp
HEADERS = ../theplugin/plugininterface.h

win32 {
    CONFIG(debug, debug|release) {
        TARGET = ../../debug/tst_qpluginloader
    } else {
        TARGET = ../../release/tst_qpluginloader
    }
}

TESTDATA += ../elftest

mac: CONFIG += insignificant_test # QTBUG-22765
