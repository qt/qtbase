TARGET = bb10styleplugin

PLUGIN_TYPE = styles
PLUGIN_CLASS_NAME = BlackBerry10StylePlugin
load(qt_plugin)

INCLUDEPATH += $$PWD

QT += widgets

HEADERS += \
    qpixmapstyle.h \
    qbb10brightstyle.h \
    qbb10darkstyle.h \
    qbb10styleplugin.h

SOURCES += \
    qpixmapstyle.cpp \
    qbb10brightstyle.cpp \
    qbb10darkstyle.cpp \
    qbb10styleplugin.cpp

RESOURCES += \
    qbb10brightstyle.qrc \
    qbb10darkstyle.qrc

OTHER_FILES += qbb10styleplugin.json

