TEMPLATE = app

OBJECTIVE_SOURCES += main.mm
HEADERS += window.h
SOURCES += window.cpp
LIBS += -framework Cocoa

QMAKE_INFO_PLIST = Info_mac.plist
OTHER_FILES = Info_mac.plist
QT += gui widgets widgets-private gui-private core-private

QT += declarative
