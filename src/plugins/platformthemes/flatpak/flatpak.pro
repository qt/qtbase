TARGET = qflatpak

PLUGIN_TYPE = platformthemes
PLUGIN_EXTENDS = -
PLUGIN_CLASS_NAME = QFlatpakThemePlugin
load(qt_plugin)

QT += core-private dbus gui-private theme_support-private

HEADERS += \
        qflatpaktheme.h \
        qflatpakfiledialog_p.h

SOURCES += \
        main.cpp \
        qflatpaktheme.cpp \
        qflatpakfiledialog.cpp
