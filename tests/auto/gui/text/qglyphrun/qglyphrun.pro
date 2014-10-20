CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qglyphrun
QT = core gui testlib

linux: CONFIG += insignificant_test

SOURCES += \
    tst_qglyphrun.cpp


wince* {
    additionalFiles.files = test.ttf
    additionalFiles.path = ../../../shared/resources/
    DEPLOYMENT += additionalFiles
} else {
    RESOURCES += \
        testdata.qrc
}

