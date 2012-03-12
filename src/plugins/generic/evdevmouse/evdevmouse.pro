TARGET = qevdevmouseplugin
load(qt_plugin)

DESTDIR = $$QT.gui.plugins/generic
target.path = $$[QT_INSTALL_PLUGINS]/generic
INSTALLS += target

HEADERS = qevdevmousehandler.h \
    qevdevmousemanager.h

QT += core-private platformsupport-private

SOURCES = main.cpp \
          qevdevmousehandler.cpp \
    qevdevmousemanager.cpp

OTHER_FILES += \
    evdevmouse.json

LIBS += $$QMAKE_LIBS_LIBUDEV
