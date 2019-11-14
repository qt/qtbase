#This project can be used to verify QTBUG-36152 case.
QT       += core gui printsupport widgets
CONFIG   += c++11
TARGET = qtexttableborders
TEMPLATE = app
SOURCES  += main.cpp widget.cpp
HEADERS  += widget.h
FORMS    += widget.ui
RESOURCES += resources.qrc
