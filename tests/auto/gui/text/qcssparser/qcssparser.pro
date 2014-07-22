CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qcssparser
SOURCES += tst_qcssparser.cpp
QT += xml gui-private testlib

requires(contains(QT_CONFIG,private_tests))
DEFINES += SRCDIR=\\\"$$PWD\\\"

wince* {
   addFiles.files = testdata
   addFiles.path = .
   timesFont.files = C:/Windows/Fonts/times.ttf
   timesFont.path = .
   DEPLOYMENT += addFiles timesFont
}

android {
    RESOURCES += \
        testdata.qrc

}
