CONFIG += testcase
qtConfig(c++11): CONFIG += c++11
qtConfig(c++14): CONFIG += c++14
debug_and_release {
    CONFIG(debug, debug|release) {
        TARGET = ../../debug/tst_qlogging
        !android:!winrt: TEST_HELPER_INSTALLS = ../debug/helper
    } else {
        TARGET = ../../release/tst_qlogging
        !android:!winrt: TEST_HELPER_INSTALLS = ../release/helper
    }
} else {
    TARGET = ../tst_qlogging
    !android:!winrt: TEST_HELPER_INSTALLS = ../helper
}

QT = core testlib
SOURCES = ../tst_qlogging.cpp

DEFINES += QT_MESSAGELOGCONTEXT
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
