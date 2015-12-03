QT += core gui

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += widgets
    qtHaveModule(printsupport): QT += printsupport
}

TARGET = dialogs
TEMPLATE = app

SOURCES += main.cpp filedialogpanel.cpp colordialogpanel.cpp fontdialogpanel.cpp \
    wizardpanel.cpp messageboxpanel.cpp utils.cpp
HEADERS += filedialogpanel.h colordialogpanel.h fontdialogpanel.h \
    wizardpanel.h messageboxpanel.h utils.h

!contains(DEFINES, QT_NO_PRINTER) {
    SOURCES += printdialogpanel.cpp
    HEADERS += printdialogpanel.h
    FORMS += printdialogpanel.ui
}
