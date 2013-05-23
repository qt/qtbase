#This project can be used to verify QTBUG-5111 case.
QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TARGET = qtexteditlist
TEMPLATE = app
SOURCES  += main.cpp widget.cpp
HEADERS  += widget.h
FORMS    += widget.ui
