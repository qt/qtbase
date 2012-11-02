TARGET = windowsprintersupport
MODULE = windowsprintersupport
PLUGIN_TYPE = printsupport
load(qt_plugin)

QT *= core-private
QT *= gui-private
QT *= printsupport-private

INCLUDEPATH *= $$QT_SOURCE_TREE/src/printsupport/kernel

SOURCES += \
    main.cpp \
    qwindowsprintersupport.cpp

HEADERS += \
    qwindowsprintersupport.h

OTHER_FILES += windows.json

LIBS += -lwinspool -lcomdlg32
