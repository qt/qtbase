TEMPLATE = app
TARGET = star
CONFIG += qt debug warn_on
QT += openvg
SOURCES = starwidget.cpp main.cpp
HEADERS = starwidget.h
LIBS += $$QMAKE_LIBS_OPENVG