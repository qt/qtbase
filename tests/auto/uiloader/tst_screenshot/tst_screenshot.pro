TEMPLATE = app

DESTDIR = ./
INCLUDEPATH += .
!embedded:CONFIG += uitools
TARGET = tst_screenshot

SOURCES += main.cpp
