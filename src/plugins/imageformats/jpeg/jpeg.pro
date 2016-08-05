TARGET  = qjpeg

QT += core-private gui-private

SOURCES += main.cpp qjpeghandler.cpp
HEADERS += main.h qjpeghandler_p.h

qtConfig(system-jpeg) {
    QMAKE_USE += libjpeg
} else {
    include($$PWD/../../../3rdparty/libjpeg.pri)
}

OTHER_FILES += jpeg.json

PLUGIN_TYPE = imageformats
PLUGIN_CLASS_NAME = QJpegPlugin
load(qt_plugin)
