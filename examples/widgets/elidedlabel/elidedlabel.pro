# Nokia Qt Examples: elided label example

QT += core gui widgets

TARGET = elidedlabel
TEMPLATE = app

SOURCES += \
    main.cpp\
    testwidget.cpp \
    elidedlabel.cpp

HEADERS += \
    testwidget.h \
    elidedlabel.h

CONFIG += mobility
MOBILITY =
