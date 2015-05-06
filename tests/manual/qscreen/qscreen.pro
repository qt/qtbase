QT += core gui widgets
CONFIG += console

TARGET = qscreen
TEMPLATE = app

SOURCES += main.cpp \
    propertywatcher.cpp \
    propertyfield.cpp

HEADERS  += \
    propertywatcher.h \
    propertyfield.h
