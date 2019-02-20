QT += core
QT -= gui

TARGET = cbordump
CONFIG += cmdline

TEMPLATE = app

# install
target.path = $$[QT_INSTALL_EXAMPLES]/corelib/serialization/cbordump
INSTALLS += target

SOURCES += main.cpp
