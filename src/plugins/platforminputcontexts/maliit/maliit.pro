TARGET = maliitplatforminputcontextplugin

PLUGIN_TYPE = platforminputcontexts
PLUGIN_CLASS_NAME = QMaliitPlatformInputContextPlugin
load(qt_plugin)

QT += dbus gui-private
SOURCES += $$PWD/qmaliitplatforminputcontext.cpp \
           $$PWD/serverproxy.cpp \
           $$PWD/serveraddressproxy.cpp \
           $$PWD/contextadaptor.cpp \
           $$PWD/main.cpp

HEADERS += $$PWD/qmaliitplatforminputcontext.h \
           $$PWD/serverproxy.h \
           $$PWD/serveraddressproxy.h \
           $$PWD/contextadaptor.h

OTHER_FILES += $$PWD/maliit.json
