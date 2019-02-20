SOURCES += crashOnExit.cpp
debug_and_release {
    CONFIG(debug, debug|release) {
        TARGET = ../../debug/crashOnExit_helper
    } else {
        TARGET = ../../release/crashOnExit_helper
    }
} else {
    TARGET = ../crashOnExit_helper
}
QT = core
CONFIG += cmdline

# This app is testdata for tst_qthreadstorage
target.path = $$[QT_INSTALL_TESTS]/tst_qthreadstorage/$$TARGET
INSTALLS += target
