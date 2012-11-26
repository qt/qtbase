#! [0]
TEMPLATE    = lib
CONFIG     += plugin
HEADERS     = simplestyle.h \
              simplestyleplugin.h
SOURCES     = simplestyle.cpp \
              simplestyleplugin.cpp
OTHER_FILES += simplestyle.json
TARGET      = simplestyleplugin
#! [0]
win32 {
    CONFIG(debug, release|debug):DESTDIR = ../debug/styles/
    CONFIG(release, release|debug):DESTDIR = ../release/styles/
} else {
    DESTDIR = ../styles/
}

# install
target.path = $$[QT_INSTALL_EXAMPLES]/tools/styleplugin/styles
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS plugin.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/tools/styleplugin/plugin
INSTALLS += target sources


QT += widgets
