TARGET = qtlibjpeg

CONFIG += \
    static \
    hide_symbols \
    exceptions_off rtti_off warn_off \
    installed

MODULE_INCLUDEPATH = $$PWD/src
MODULE_EXT_HEADERS = $$PWD/src/jpeglib.h \
                     $$PWD/src/jerror.h \
                     $$PWD/src/jconfig.h \
                     $$PWD/src/jmorecfg.h

INCLUDEPATH += $$PWD $$PWD/src

load(qt_helper_lib)

winrt: DEFINES += NO_GETENV

# Disable warnings in 3rdparty code due to unused arguments
gcc: QMAKE_CFLAGS_WARN_ON += -Wno-unused-parameter -Wno-main

# Do not warn about sprintf, getenv, sscanf ... use
msvc: DEFINES += _CRT_SECURE_NO_WARNINGS

SOURCES += \
    $$PWD/src/jaricom.c \
    $$PWD/src/jcapimin.c \
    $$PWD/src/jcapistd.c \
    $$PWD/src/jcarith.c \
    $$PWD/src/jccoefct.c \
    $$PWD/src/jccolor.c \
    $$PWD/src/jcdctmgr.c \
    $$PWD/src/jchuff.c \
    $$PWD/src/jcinit.c \
    $$PWD/src/jcmainct.c \
    $$PWD/src/jcmarker.c \
    $$PWD/src/jcmaster.c \
    $$PWD/src/jcomapi.c \
    $$PWD/src/jcparam.c \
    $$PWD/src/jcprepct.c \
    $$PWD/src/jcsample.c \
    $$PWD/src/jctrans.c \
    $$PWD/src/jdapimin.c \
    $$PWD/src/jdapistd.c \
    $$PWD/src/jdarith.c \
    $$PWD/src/jdatadst.c \
    $$PWD/src/jdatasrc.c \
    $$PWD/src/jdcoefct.c \
    $$PWD/src/jdcolor.c \
    $$PWD/src/jddctmgr.c \
    $$PWD/src/jdhuff.c \
    $$PWD/src/jdinput.c \
    $$PWD/src/jdmainct.c \
    $$PWD/src/jdmarker.c \
    $$PWD/src/jdmaster.c \
    $$PWD/src/jdmerge.c \
    $$PWD/src/jdpostct.c \
    $$PWD/src/jdsample.c \
    $$PWD/src/jdtrans.c \
    $$PWD/src/jerror.c \
    $$PWD/src/jfdctflt.c \
    $$PWD/src/jfdctfst.c \
    $$PWD/src/jfdctint.c \
    $$PWD/src/jidctflt.c \
    $$PWD/src/jidctfst.c \
    $$PWD/src/jidctint.c \
    $$PWD/src/jquant1.c \
    $$PWD/src/jquant2.c \
    $$PWD/src/jutils.c \
    $$PWD/src/jmemmgr.c \
    $$PWD/src/jsimd_none.c \
    $$PWD/src/jcphuff.c \
    $$PWD/src/jidctred.c \
    $$PWD/src/jdphuff.c \
    $$PWD/src/jmemnobs.c

TR_EXCLUDE += $$PWD/*
