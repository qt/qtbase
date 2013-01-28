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
    DESTDIR = ../styles/
}

EXAMPLE_FILES += simplestyle.json

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/tools/styleplugin/styles
INSTALLS += target
