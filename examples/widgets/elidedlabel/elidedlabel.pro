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
    include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
}

maemo5: include($$QT_SOURCE_TREE/examples/maemo5pkgrules.pri)

symbian: warning(This example might not fully work on Symbian platform)
maemo5: warning(This example might not fully work on Maemo platform)
simulator: warning(This example might not fully work on Simulator platform)
