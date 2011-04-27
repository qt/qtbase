TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .
QT -= gui
CONFIG += qdbus
win32:CONFIG += console

# Input
SOURCES += listnames.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/dbus/listnames
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/dbus/listnames
INSTALLS += target sources

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)

