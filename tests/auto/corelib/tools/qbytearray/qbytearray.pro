CONFIG += testcase parallel_test
TARGET = tst_qbytearray
QT = core-private testlib
SOURCES = tst_qbytearray.cpp

TESTDATA += rfc3252.txt
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

mac {
    OBJECTIVE_SOURCES += tst_qbytearray_mac.mm
    LIBS += -framework Foundation
}

android: !android-no-sdk {
    RESOURCES += \
        android_testdata.qrc
}
