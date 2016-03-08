CONFIG += testcase
TARGET = tst_qcssparser
SOURCES += tst_qcssparser.cpp
QT += xml gui-private testlib

requires(contains(QT_CONFIG,private_tests))
DEFINES += SRCDIR=\\\"$$PWD\\\"

android {
    RESOURCES += \
        testdata.qrc

}
