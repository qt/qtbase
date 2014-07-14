CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qfontdatabase
SOURCES  += tst_qfontdatabase.cpp
QT += testlib core-private gui-private

wince* {
    additionalFiles.files = FreeMono.ttf
    additionalFiles.path = .
    DEPLOYMENT += additionalFiles
}

android {
    RESOURCES += testdata.qrc
}

