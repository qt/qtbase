TEMPLATE = app
QT += gui-private
CONFIG += console c++11
CONFIG -= app_bundle
SOURCES += main.cpp itemwindow.cpp
HEADERS += itemwindow.h
include(../diaglib/diaglib.pri)
