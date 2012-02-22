TARGET = qevdevtouchplugin
load(qt_plugin)

DESTDIR = $$QT.gui.plugins/generic
target.path = $$[QT_INSTALL_PLUGINS]/generic
INSTALLS += target

HEADERS = \
    qevdevtouch.h

SOURCES = main.cpp \
    qevdevtouch.cpp

QT += core-private platformsupport-private

OTHER_FILES += \
    evdevtouch.json
