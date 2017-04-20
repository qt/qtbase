TARGET = qgtk3

PLUGIN_TYPE = platformthemes
PLUGIN_EXTENDS = -
PLUGIN_CLASS_NAME = QGtk3ThemePlugin
load(qt_plugin)

QT += core-private gui-private theme_support-private

CONFIG += X11
QMAKE_USE += gtk3
DEFINES += GDK_VERSION_MIN_REQUIRED=GDK_VERSION_3_6

HEADERS += \
        qgtk3dialoghelpers.h \
        qgtk3menu.h \
        qgtk3theme.h

SOURCES += \
        main.cpp \
        qgtk3dialoghelpers.cpp \
        qgtk3menu.cpp \
        qgtk3theme.cpp
