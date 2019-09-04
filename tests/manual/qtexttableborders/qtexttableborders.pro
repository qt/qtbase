#This project can be used to verify QTBUG-36152 case.
QT       += core gui printsupport
CONFIG   += c++11
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TARGET = qtexttableborders
TEMPLATE = app
SOURCES  += main.cpp widget.cpp
HEADERS  += widget.h
FORMS    += widget.ui
RESOURCES += resources.qrc
