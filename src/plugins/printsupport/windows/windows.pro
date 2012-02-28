TARGET = windows
load(qt_plugin)

QT *= core-private
QT *= gui-private
QT *= printsupport-private

QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/printsupport
INCLUDEPATH *= $$QT_SOURCE_TREE/src/printsupport/kernel

SOURCES += \
    main.cpp \
    qwindowsprintersupport.cpp

HEADERS += \
    qwindowsprintersupport.h

OTHER_FILES += windows.json

target.path += $$[QT_INSTALL_PLUGINS]/printsupport
INSTALLS += target
LIBS += -lWinspool -lComdlg32
