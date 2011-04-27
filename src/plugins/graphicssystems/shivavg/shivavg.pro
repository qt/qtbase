TARGET = qshivavggraphicssystem
include(../../qpluginbase.pri)

QT += openvg

QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/graphicssystems

SOURCES = main.cpp shivavggraphicssystem.cpp shivavgwindowsurface.cpp
HEADERS = shivavggraphicssystem.h shivavgwindowsurface.h

target.path += $$[QT_INSTALL_PLUGINS]/graphicssystems
INSTALLS += target
