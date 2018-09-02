TARGET = qxdgdesktopportal

PLUGIN_TYPE = platformthemes
PLUGIN_EXTENDS = -
PLUGIN_CLASS_NAME = QXdgDesktopPortalThemePlugin
load(qt_plugin)

QT += core-private dbus gui-private theme_support-private

HEADERS += \
        qxdgdesktopportaltheme.h \
        qxdgdesktopportalfiledialog_p.h

SOURCES += \
        main.cpp \
        qxdgdesktopportaltheme.cpp \
        qxdgdesktopportalfiledialog.cpp
