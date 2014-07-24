CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qglyphrun
QT = core gui testlib

linux: CONFIG += insignificant_test

SOURCES += \
    tst_qglyphrun.cpp

android {
    RESOURCES += \
        testdata.qrc
}

wince* {
    additionalFiles.files = test.ttf
    additionalFiles.path = .
    DEPLOYMENT += additionalFiles
}

