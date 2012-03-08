SOURCES = qdbuscpp2xml.cpp
DESTDIR = $$QT.designer.bins
TARGET = qdbuscpp2xml
QT = core
CONFIG += qdbus
CONFIG -= app_bundle
win32:CONFIG += console

target.path=$$[QT_INSTALL_BINS]
INSTALLS += target
