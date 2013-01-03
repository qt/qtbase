TEMPLATE = app
TARGET = qmaccocoaviewcontainer
INCLUDEPATH += .
QT += widgets
LIBS += -framework Cocoa
# Input
OBJECTIVE_SOURCES += main.mm TestMouseMovedNSView.m
HEADERS += TestMouseMovedNSView.h
