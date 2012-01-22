TARGET  = qgif
load(qt_plugin)

include(../../../gui/image/qgifhandler.pri)
SOURCES += $$PWD/main.cpp

DESTDIR = $$QT.gui.plugins/imageformats
target.path += $$[QT_INSTALL_PLUGINS]/imageformats
INSTALLS += target
