TEMPLATE = app
CONFIG += benchmark
QT = core testlib
win32: DEFINES+= _CRT_SECURE_NO_WARNINGS

TARGET = tst_qurl
SOURCES += main.cpp
