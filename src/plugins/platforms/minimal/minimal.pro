TARGET = qminimal
load(qt_plugin)

QT = core-private gui-private
DESTDIR = $$QT.gui.plugins/platforms

SOURCES =   main.cpp \
            qminimalintegration.cpp \
            qminimalbackingstore.cpp
HEADERS =   qminimalintegration.h \
            qminimalbackingstore.h

target.path += $$[QT_INSTALL_PLUGINS]/platforms
INSTALLS += target
