CONFIG += testcase
TARGET = tst_qcssparser
SOURCES += tst_qcssparser.cpp
QT += xml gui-private testlib

requires(qtConfig(private_tests))
DEFINES += SRCDIR=\\\"$$PWD\\\"

android {
    RESOURCES += \
        testdata.qrc

}
