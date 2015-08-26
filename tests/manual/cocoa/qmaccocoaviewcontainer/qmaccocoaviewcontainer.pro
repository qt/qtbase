TEMPLATE = app
TARGET = qmaccocoaviewcontainer
INCLUDEPATH += .
QT += widgets
LIBS += -framework AppKit
# Input
OBJECTIVE_SOURCES += main.mm TestMouseMovedNSView.m
HEADERS += TestMouseMovedNSView.h
