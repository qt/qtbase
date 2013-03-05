QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = dialogs
TEMPLATE = app

SOURCES += main.cpp filedialogpanel.cpp colordialogpanel.cpp fontdialogpanel.cpp \
    wizardpanel.cpp
HEADERS += filedialogpanel.h colordialogpanel.h fontdialogpanel.h \
    wizardpanel.h
