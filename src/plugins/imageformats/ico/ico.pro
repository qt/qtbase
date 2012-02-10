TARGET  = qico
load(qt_plugin)

QTDIR_build:REQUIRES = "!contains(QT_CONFIG, no-ico)"

HEADERS += qicohandler.h main.h
SOURCES += main.cpp \
           qicohandler.cpp
OTHER_FILES += ico.json

DESTDIR = $$QT.gui.plugins/imageformats
target.path += $$[QT_INSTALL_PLUGINS]/imageformats
INSTALLS += target
