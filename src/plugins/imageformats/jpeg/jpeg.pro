TARGET  = qjpeg

QT += core-private

SOURCES += main.cpp qjpeghandler.cpp
HEADERS += main.h qjpeghandler_p.h

contains(QT_CONFIG, system-jpeg) {
    msvc: \
        LIBS += libjpeg.lib
    else: \
        LIBS += -ljpeg
} else {
    include($$PWD/../../../3rdparty/libjpeg.pri)
}

OTHER_FILES += jpeg.json

PLUGIN_TYPE = imageformats
PLUGIN_CLASS_NAME = QJpegPlugin
load(qt_plugin)
