TEMPLATE = app
QT += gui-private
CONFIG += cmdline c++11
SOURCES += main.cpp itemwindow.cpp
HEADERS += itemwindow.h
include(../diaglib/diaglib.pri)
