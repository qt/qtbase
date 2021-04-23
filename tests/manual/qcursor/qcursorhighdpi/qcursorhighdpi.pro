TEMPLATE = app
QT = core gui gui-private core-private widgets
CONFIG -= app_bundle
SOURCES += main.cpp
win32: LIBS += -luser32
