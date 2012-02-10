TARGET  = qjpeg
load(qt_plugin)

QT += core-private

QTDIR_build:REQUIRES = "!contains(QT_CONFIG, no-jpeg)"

include(../../../gui/image/qjpeghandler.pri)
SOURCES += main.cpp
HEADERS += main.h
OTHER_FILES += jpeg.json

DESTDIR = $$QT.gui.plugins/imageformats
target.path += $$[QT_INSTALL_PLUGINS]/imageformats
INSTALLS += target
