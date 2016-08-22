CONFIG += testcase
TARGET  = ../tst_qpluginloader
QT = core testlib
qtConfig(private_tests): QT += core-private
SOURCES = ../tst_qpluginloader.cpp ../fakeplugin.cpp
HEADERS = ../theplugin/plugininterface.h
CONFIG -= app_bundle

win32 {
    CONFIG(debug, debug|release) {
        TARGET = ../../debug/tst_qpluginloader
    } else {
        TARGET = ../../release/tst_qpluginloader
    }
}

TESTDATA += ../elftest ../machtest
