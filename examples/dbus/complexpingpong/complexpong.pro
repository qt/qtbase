TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .
QT -= gui
CONFIG += qdbus

# Input
HEADERS += complexpong.h
SOURCES += complexpong.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/dbus/complexpingpong
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/dbus/complexpingpong
INSTALLS += target sources

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
