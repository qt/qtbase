TARGET = qtlibpng

CONFIG += \
    static \
    hide_symbols \
    exceptions_off rtti_off warn_off \
    installed

MODULE_INCLUDEPATH = $$PWD

load(qt_helper_lib)

DEFINES += PNG_ARM_NEON_OPT=0
SOURCES += \
    png.c \
    pngerror.c \
    pngget.c \
    pngmem.c \
    pngpread.c \
    pngread.c \
    pngrio.c \
    pngrtran.c \
    pngrutil.c \
    pngset.c \
    pngtrans.c \
    pngwio.c \
    pngwrite.c \
    pngwtran.c \
    pngwutil.c

TR_EXCLUDE += $$PWD/*

include(../zlib_dependency.pri)
