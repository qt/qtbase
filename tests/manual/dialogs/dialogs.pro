QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = dialogs
TEMPLATE = app

SOURCES += main.cpp filedialogpanel.cpp colordialogpanel.cpp fontdialogpanel.cpp \
    wizardpanel.cpp messageboxpanel.cpp printdialogpanel.cpp  utils.cpp
HEADERS += filedialogpanel.h colordialogpanel.h fontdialogpanel.h \
    wizardpanel.h messageboxpanel.h printdialogpanel.h utils.h
FORMS += printdialogpanel.ui
