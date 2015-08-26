TEMPLATE = app

OBJECTIVE_SOURCES += main.mm
HEADERS += rasterwindow.h
SOURCES += rasterwindow.cpp
LIBS += -framework AppKit

QT += gui widgets quick

QT += quick
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
