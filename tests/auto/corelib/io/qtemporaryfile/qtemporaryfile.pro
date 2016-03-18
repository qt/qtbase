CONFIG += testcase
TARGET = tst_qtemporaryfile
QT = core testlib
SOURCES = tst_qtemporaryfile.cpp
TESTDATA += tst_qtemporaryfile.cpp
RESOURCES += qtemporaryfile.qrc

android {
    RESOURCES += android_testdata.qrc
}
