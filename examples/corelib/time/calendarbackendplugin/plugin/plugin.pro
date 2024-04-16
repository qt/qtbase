TEMPLATE = lib
TARGET = calendarPlugin
INCLUDEPATH += . \
    ../common/
QT += core core-private widgets

HEADERS += calendarbackend.h calendarplugin.h
SOURCES += calendarbackend.cpp calendarplugin.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/corelib/datetime/calendarbackendplugin/plugin
INSTALLS += target
