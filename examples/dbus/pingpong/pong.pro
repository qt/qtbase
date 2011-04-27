TEMPLATE = app
TARGET = pong
DEPENDPATH += .
INCLUDEPATH += .
QT -= gui
CONFIG += qdbus

# Input
HEADERS += ping-common.h pong.h
SOURCES += pong.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/dbus/pingpong
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/dbus/pingpong
INSTALLS += target sources

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
