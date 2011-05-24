#-------------------------------------------------
#
# Project created by QtCreator 2010-06-14T10:09:27
#
#-------------------------------------------------

QT       += core gui

TARGET = symbianvibration
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
        vibrationsurface.cpp \
        xqvibra.cpp \
        xqvibra_p.cpp

HEADERS  += mainwindow.h \
        vibrationsurface.h \
        xqvibra.h \
        xqvibra_p.h

CONFIG += mobility
MOBILITY = 

symbian {
    TARGET.UID3 = 0xecf47018
    # TARGET.CAPABILITY += 
    TARGET.EPOCSTACKSIZE = 0x14000
    TARGET.EPOCHEAPSIZE = 0x020000 0x800000
    LIBS += -lhwrmvibraclient
    include($$PWD/../../symbianpkgrules.pri)
}

!symbian {
    error(The Symbian Vibration Example only works for the Symbian target!)
}

