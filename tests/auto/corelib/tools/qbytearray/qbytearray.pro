CONFIG += testcase
TARGET = tst_qbytearray
QT = core-private testlib
SOURCES = tst_qbytearray.cpp

TESTDATA += rfc3252.txt

mac {
    OBJECTIVE_SOURCES += tst_qbytearray_mac.mm
    LIBS += -framework Foundation
}

android {
    RESOURCES += \
        android_testdata.qrc
}
