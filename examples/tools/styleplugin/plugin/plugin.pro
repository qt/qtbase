#! [0]
TEMPLATE    = lib
CONFIG     += plugin
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
    DESTDIR = ../styles/
}

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/styleplugin/styles
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS plugin.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/styleplugin/plugin
INSTALLS += target sources

symbian: CONFIG += qt_example

symbian:TARGET.EPOCALLOWDLLDATA = 1
maemo5: include($$QT_SOURCE_TREE/examples/maemo5pkgrules.pri)
