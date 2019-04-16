CONFIG += testcase
TARGET = tst_qdir
QT = core core-private testlib
SOURCES = tst_qdir.cpp
RESOURCES += qdir.qrc
ios: QMAKE_INFO_PLIST = Info.plist

TESTDATA += testdir testData searchdir resources entrylist types tst_qdir.cpp

contains(CONFIG, builtin_testdata): DEFINES += BUILTIN_TESTDATA

android:!android-embedded {
    RESOURCES += android_testdata.qrc
}
