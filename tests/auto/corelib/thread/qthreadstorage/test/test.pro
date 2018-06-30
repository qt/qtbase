CONFIG += testcase
debug_and_release {
    CONFIG(debug, debug|release) {
        TARGET = ../../debug/tst_qthreadstorage
        !android:!winrt: TEST_HELPER_INSTALLS = ../../debug/crashonexit_helper
    } else {
        TARGET = ../../release/tst_qthreadstorage
        !android:!winrt: TEST_HELPER_INSTALLS = ../../release/crashonexit_helper
    }
} else {
    TARGET = ../tst_qthreadstorage
    !android:!winrt: TEST_HELPER_INSTALLS = ../crashonexit_helper
}
CONFIG += console
QT = core testlib
SOURCES = ../tst_qthreadstorage.cpp
