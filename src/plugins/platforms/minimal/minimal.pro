TARGET = qminimal
include(../../qpluginbase.pri)
QT += gui-private

DESTDIR = $$QT.gui.plugins/platforms

SOURCES =   main.cpp \
            qminimalintegration.cpp \
            qminimalwindowsurface.cpp
HEADERS =   qminimalintegration.h \
            qminimalwindowsurface.h

target.path += $$[QT_INSTALL_PLUGINS]/platforms
INSTALLS += target
