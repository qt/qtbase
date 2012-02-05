TARGET = qevdevmouseplugin
load(qt_plugin)

DESTDIR = $$QT.gui.plugins/generic
target.path = $$[QT_INSTALL_PLUGINS]/generic
INSTALLS += target

HEADERS = qevdevmouse.h

QT += core-private platformsupport-private

SOURCES = main.cpp \
          qevdevmouse.cpp
