CONFIG -= app_bundle
CONFIG += console
debug_and_release {
    CONFIG(debug, debug|release) {
        TARGET = ../../debug/signalbug_helper
    } else {
        TARGET = ../../release/signalbug_helper
    }
} else {
    TARGET = ../signalbug_helper
}

QT = core

HEADERS += signalbug.h
SOURCES += signalbug.cpp

# This app is testdata for tst_qobject
target.path = $$[QT_INSTALL_TESTS]/tst_qobject/$$TARGET
INSTALLS += target
