TARGET = qglgraphicssystem
load(qt_plugin)

QT += core-private gui-private opengl-private

DESTDIR = $$QT.gui.plugins/graphicssystems

SOURCES = main.cpp

target.path += $$[QT_INSTALL_PLUGINS]/graphicssystems
INSTALLS += target

symbian: TARGET.UID3 = 0x2002131B
