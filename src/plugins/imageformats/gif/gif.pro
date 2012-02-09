TARGET  = qgif
load(qt_plugin)

include(../../../gui/image/qgifhandler.pri)
SOURCES += $$PWD/main.cpp
HEADERS += $$PWD/main.h
OTHER_FILES += gif.json

DESTDIR = $$QT.gui.plugins/imageformats
target.path += $$[QT_INSTALL_PLUGINS]/imageformats
INSTALLS += target
