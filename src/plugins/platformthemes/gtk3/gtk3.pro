TARGET = qgtk3

PLUGIN_TYPE = platformthemes
PLUGIN_EXTENDS = -
PLUGIN_CLASS_NAME = QGtk3ThemePlugin
load(qt_plugin)

QT += core-private gui-private platformsupport-private

CONFIG += X11
QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_GTK3
LIBS += $$QMAKE_LIBS_GTK3

HEADERS += \
        qgtk3dialoghelpers.h \
        qgtk3theme.h

SOURCES += \
        main.cpp \
        qgtk3dialoghelpers.cpp \
        qgtk3theme.cpp
