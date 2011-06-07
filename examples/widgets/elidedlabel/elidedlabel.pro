# Nokia Qt Examples: elided label example

QT += core gui

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

symbian {
    TARGET.UID3 = 0xE2728354 # randomly generated
    TARGET.EPOCSTACKSIZE = 0x14000
    TARGET.EPOCHEAPSIZE = 0x020000 0x800000
    CONFIG += qt_example
}

maemo5: CONFIG += qt_example

symbian: warning(This example might not fully work on Symbian platform)
maemo5: warning(This example might not fully work on Maemo platform)
simulator: warning(This example might not fully work on Simulator platform)
