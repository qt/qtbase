TARGET  = qico
include(../../qpluginbase.pri)

QTDIR_build:REQUIRES = "!contains(QT_CONFIG, no-ico)"

HEADERS += qicohandler.h
SOURCES += main.cpp \
           qicohandler.cpp

DESTDIR = $$QT.gui.plugins/imageformats
target.path += $$[QT_INSTALL_PLUGINS]/imageformats
INSTALLS += target

symbian:TARGET.UID3=0x2001E616
