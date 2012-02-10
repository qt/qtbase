TARGET = qevdevkeyboardplugin
load(qt_plugin)

DESTDIR = $$QT.gui.plugins/generic
target.path = $$[QT_INSTALL_PLUGINS]/generic
INSTALLS += target

HEADERS = \
    qevdevkeybaord_defaultmap.h \
    qevdevkeyboardhandler.h \
    qevdevkeyboardmanager.h

QT += core-private

LIBS += -ludev

SOURCES = main.cpp \
    qevdevkeyboardhandler.cpp \
    qevdevkeyboardmanager.cpp
