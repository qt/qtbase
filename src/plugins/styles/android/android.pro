TARGET = qandroidstyle

QT += widgets-private

SOURCES += \
    main.cpp \
    qandroidstyle.cpp

HEADERS += \
    qandroidstyle_p.h

DISTFILES += androidstyle.json

PLUGIN_TYPE = styles
PLUGIN_CLASS_NAME = QAndroidStylePlugin
load(qt_plugin)
