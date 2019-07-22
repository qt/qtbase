CONFIG += testcase
TARGET  = ../tst_qpluginloader
QT = core testlib
qtConfig(private_tests): QT += core-private
SOURCES = ../tst_qpluginloader.cpp ../fakeplugin.cpp
HEADERS = ../theplugin/plugininterface.h

win32:debug_and_release {
    CONFIG(debug, debug|release) {
        TARGET = ../../debug/tst_qpluginloader
        LIBS += -L../staticplugin/debug
    } else {
        TARGET = ../../release/tst_qpluginloader
        LIBS += -L../staticplugin/release
    }
} else {
    LIBS += -L../staticplugin
}
LIBS += -lstaticplugin

TESTDATA += ../elftest ../machtest
