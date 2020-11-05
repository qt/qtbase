CONFIG += testcase
qtConfig(c++17): CONFIG += c++17
debug_and_release {
    CONFIG(debug, debug|release) {
        TARGET = ../../debug/tst_qmessagelogger
    } else {
        TARGET = ../../release/tst_qmessagelogger
    }
} else {
    TARGET = ../tst_qmessagelogger
}

QT = core testlib
SOURCES = ../tst_qmessagelogger.cpp

DEFINES += QT_MESSAGELOGCONTEXT
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
