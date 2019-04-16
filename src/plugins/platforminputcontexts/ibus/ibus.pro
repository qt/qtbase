TARGET = ibusplatforminputcontextplugin

QT += dbus gui-private xkbcommon_support-private
SOURCES += $$PWD/qibusplatforminputcontext.cpp \
           $$PWD/qibusproxy.cpp \
           $$PWD/qibusproxyportal.cpp \
           $$PWD/qibusinputcontextproxy.cpp \
           $$PWD/qibustypes.cpp \
           $$PWD/main.cpp

HEADERS += $$PWD/qibusplatforminputcontext.h \
           $$PWD/qibusproxy.h \
           $$PWD/qibusproxyportal.h \
           $$PWD/qibusinputcontextproxy.h \
           $$PWD/qibustypes.h

OTHER_FILES += $$PWD/ibus.json

PLUGIN_TYPE = platforminputcontexts
PLUGIN_EXTENDS = -
PLUGIN_CLASS_NAME = QIbusPlatformInputContextPlugin
load(qt_plugin)
