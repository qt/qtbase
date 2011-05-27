TARGET = qshivavggraphicssystem
load(qt_plugin)

QT += openvg

DESTDIR = $$QT.gui.plugins/graphicssystems

SOURCES = main.cpp shivavggraphicssystem.cpp shivavgwindowsurface.cpp
HEADERS = shivavggraphicssystem.h shivavgwindowsurface.h

target.path += $$[QT_INSTALL_PLUGINS]/graphicssystems
INSTALLS += target
