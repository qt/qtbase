TEMPLATE = app
TARGET = tst_qloggingregistry

CONFIG += testcase
QT = core core-private testlib

SOURCES += tst_qloggingregistry.cpp
TESTDATA += qtlogging.ini

android:!android-embedded {
    RESOURCES += \
        android_testdata.qrc
}
