TARGET = qmeegointegration
load(qt_plugin)

QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/generic

target.path = $$[QT_INSTALL_PLUGINS]/generic
INSTALLS += target

SOURCES = qmeegointegration.cpp \
          main.cpp \
          contextkitproperty.cpp
HEADERS = qmeegointegration.h \
          contextkitproperty.h

QT = core-private gui-private dbus
