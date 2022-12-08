#! [0]
TEMPLATE    = lib
CONFIG     += plugin
QT         += widgets
HEADERS     = simplestyle.h \
              simplestyleplugin.h
SOURCES     = simplestyle.cpp \
              simplestyleplugin.cpp
TARGET      = simplestyleplugin
#! [0]
win32 {
    CONFIG(debug, release|debug):DESTDIR = ../debug/styles/
    CONFIG(release, release|debug):DESTDIR = ../release/styles/
} else {
    macos {
        # The non-app-bundle case is not supported with qmake, because
        # the plugin project cannot know whether the app is built
        # as a bundle or not.
        DESTDIR = ../styleplugin.app/Contents/PlugIns/styles/
        contains(QT_CONFIG, debug) {
            TARGET = $$join(TARGET,,,_debug)
        }
    } else {
        DESTDIR = ../styles/
    }
}

EXAMPLE_FILES += simplestyle.json

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/tools/styleplugin/styles
INSTALLS += target

CONFIG += install_ok  # Do not cargo-cult this!
