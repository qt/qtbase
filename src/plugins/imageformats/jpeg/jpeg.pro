TARGET  = qjpeg

QT += core-private gui-private

SOURCES += main.cpp qjpeghandler.cpp
HEADERS += main.h qjpeghandler_p.h

qtConfig(system-jpeg) {
    QMAKE_USE += libjpeg
} else {
    QMAKE_USE_PRIVATE += libjpeg
}

OTHER_FILES += jpeg.json

PLUGIN_TYPE = imageformats
PLUGIN_CLASS_NAME = QJpegPlugin
load(qt_plugin)
