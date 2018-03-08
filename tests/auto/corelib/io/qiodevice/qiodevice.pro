CONFIG += testcase
TARGET = tst_qiodevice
QT = core network testlib
SOURCES = tst_qiodevice.cpp

TESTDATA += tst_qiodevice.cpp
MOC_DIR=tmp

android:!android-embedded {
    RESOURCES += \
        android_testdata.qrc
}
