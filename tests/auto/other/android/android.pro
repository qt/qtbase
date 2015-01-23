CONFIG += testcase
TARGET = tst_android
QT = core testlib

SOURCES += \
    tst_android.cpp

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/testdata

DISTFILES += \
    testdata/assets/test.txt
