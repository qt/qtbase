CONFIG += testcase
TARGET = tst_macnativeevents
QT += widgets testlib
HEADERS += qnativeevents.h nativeeventlist.h expectedeventlist.h
SOURCES += qnativeevents.cpp qnativeevents_mac.cpp
SOURCES += expectedeventlist.cpp nativeeventlist.cpp
SOURCES += tst_macnativeevents.cpp

requires(mac)
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

LIBS += -framework AppKit
