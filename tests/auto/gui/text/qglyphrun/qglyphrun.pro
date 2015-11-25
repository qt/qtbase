CONFIG += testcase
TARGET = tst_qglyphrun
QT = core gui testlib

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

