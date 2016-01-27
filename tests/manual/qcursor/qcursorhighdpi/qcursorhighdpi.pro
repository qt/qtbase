TEMPLATE = app
QT = core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += gui-private core-private widgets
CONFIG -= app_bundle
SOURCES += main.cpp
win32: LIBS += -lUser32
