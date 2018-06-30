TARGET = QtThemeSupport
MODULE = theme_support

QT = core-private gui-private
CONFIG += static internal_module

DEFINES += QT_NO_CAST_FROM_ASCII

if(unix:!uikit)|qtConfig(xcb): \
    include($$PWD/genericunix/genericunix.pri)

HEADERS += \
    qabstractfileiconengine_p.h

SOURCES += \
    qabstractfileiconengine.cpp

load(qt_module)
