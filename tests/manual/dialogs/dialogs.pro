QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = dialogs
TEMPLATE = app

SOURCES += main.cpp filedialogpanel.cpp colordialogpanel.cpp
HEADERS += filedialogpanel.h colordialogpanel.h
