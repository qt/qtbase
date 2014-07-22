CONFIG += testcase
TARGET = tst_qzip
QT += gui-private testlib
SOURCES += tst_qzip.cpp

wince* {
   addFiles.files = testdata
   addFiles.path = .
   DEPLOYMENT += addFiles
}

android {
    RESOURCES += \
        testdata.qrc
}
