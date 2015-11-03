TARGET = qgtk3

PLUGIN_TYPE = platformthemes
PLUGIN_EXTENDS = -
PLUGIN_CLASS_NAME = QGtk3ThemePlugin
load(qt_plugin)

QT += core-private gui-private platformsupport-private

CONFIG += X11
QMAKE_CXXFLAGS += $$QT_CFLAGS_QGTK3
LIBS += $$QT_LIBS_QGTK3

HEADERS += \
        qgtk3dialoghelpers.h \
        qgtk3theme.h

SOURCES += \
        main.cpp \
        qgtk3dialoghelpers.cpp \
        qgtk3theme.cpp
