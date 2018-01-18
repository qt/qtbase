QT += core
QT -= gui

TARGET = cbordump
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

# install
target.path = $$[QT_INSTALL_EXAMPLES]/corelib/serialization/cbordump
INSTALLS += target

SOURCES += main.cpp
