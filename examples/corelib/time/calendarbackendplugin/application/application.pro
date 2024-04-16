TEMPLATE = app
TARGET = application
INCLUDEPATH += . \
    ../common/
QT += core core-private widgets

target.path = $$[QT_INSTALL_EXAMPLES]/corelib/datetime/calendarbackendplugin/application
INSTALLS += target

SOURCES += main.cpp
HEADERS += ../common/calendarBackendInterface.h
