SOURCES += main.cpp
QT = core
CONFIG -= app_bundle
CONFIG += console

debug_and_release {
    CONFIG(debug, debug|release) {
        TARGET = ../../debug/stdinprocess_helper
    } else {
        TARGET = ../../release/stdinprocess_helper
    }
} else {
    TARGET = ../stdinprocess_helper
}

# This app is testdata for tst_qfile
target.path = $$[QT_INSTALL_TESTS]/tst_qfile/$$TARGET
INSTALLS += target
