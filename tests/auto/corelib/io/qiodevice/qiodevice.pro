CONFIG += testcase
TARGET = tst_qiodevice
QT = core network testlib
SOURCES = tst_qiodevice.cpp

TESTDATA += tst_qiodevice.cpp
MOC_DIR=tmp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

android:!android-no-sdk: {
    RESOURCES += \
        android_testdata.qrc
}
