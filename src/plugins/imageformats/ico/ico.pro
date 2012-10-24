TARGET  = qico

PLUGIN_TYPE = imageformats
load(qt_plugin)

QTDIR_build:REQUIRES = "!contains(QT_CONFIG, no-ico)"

HEADERS += qicohandler.h main.h
SOURCES += main.cpp \
           qicohandler.cpp
OTHER_FILES += ico.json
