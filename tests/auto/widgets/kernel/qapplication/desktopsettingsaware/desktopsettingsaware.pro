QT += widgets
CONFIG -= app_bundle

debug_and_release {
    CONFIG(debug, debug|release) {
        TARGET = ../../debug/desktopsettingsaware_helper
    } else {
        TARGET = ../../release/desktopsettingsaware_helper
    }
} else {
    TARGET = ../desktopsettingsaware_helper
}

SOURCES += main.cpp
