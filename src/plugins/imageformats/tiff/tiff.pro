TARGET  = qtiff
load(qt_plugin)

QTDIR_build:REQUIRES = "!contains(QT_CONFIG, no-tiff)"

include(../../../gui/image/qtiffhandler.pri)
SOURCES += main.cpp

DESTDIR = $$QT.gui.plugins/imageformats
target.path += $$[QT_INSTALL_PLUGINS]/imageformats
INSTALLS += target

symbian:TARGET.UID3=0x2001E617
