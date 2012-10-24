TARGET = qmeegointegration

PLUGIN_TYPE = generic
load(qt_plugin)

SOURCES = qmeegointegration.cpp \
          main.cpp \
          contextkitproperty.cpp
HEADERS = qmeegointegration.h \
          contextkitproperty.h

QT = core-private gui-private dbus gui-private
