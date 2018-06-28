QT += widgets
SOURCES += main.cpp \
    base.cpp
debug_and_release {
    CONFIG(debug, debug|release) {
        TARGET = ../../debug/modal_helper
    } else {
        TARGET = ../../release/modal_helper
    }
} else {
    TARGET = ../modal_helper
}
CONFIG -= app_bundle
HEADERS += base.h

