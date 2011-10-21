TARGET = qtouchscreenplugin
load(qt_plugin)

DESTDIR = $$QT.gui.plugins/generic
target.path = $$[QT_INSTALL_PLUGINS]/generic
INSTALLS += target

HEADERS = \
    qtouchscreen.h \
    qtoucheventsenderqpa.h

SOURCES = main.cpp \
    qtouchscreen.cpp \
    qtoucheventsenderqpa.cpp

QT += core-private gui-private

LIBS += -ludev
