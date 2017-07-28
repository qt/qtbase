winrt: DEFINES += NO_GETENV

DEFINES += \
    C_ARITH_CODING_SUPPORTED=1 \
    D_ARITH_CODING_SUPPORTED=1 \
    BITS_IN_JSAMPLE=8 \
    JPEG_LIB_VERSION=80 \
    SIZEOF_SIZE_T=__SIZEOF_SIZE_T__

#Disable warnings in 3rdparty code due to unused arguments
contains(QMAKE_CC, gcc): {
    QMAKE_CFLAGS_WARN_ON += -Wno-unused-parameter -Wno-main
}


INCLUDEPATH += $$PWD/libjpeg/src
SOURCES += \
    $$PWD/libjpeg/src/jaricom.c \
    $$PWD/libjpeg/src/jcapimin.c \
    $$PWD/libjpeg/src/jcapistd.c \
    $$PWD/libjpeg/src/jcarith.c \
    $$PWD/libjpeg/src/jccoefct.c \
    $$PWD/libjpeg/src/jccolor.c \
    $$PWD/libjpeg/src/jcdctmgr.c \
    $$PWD/libjpeg/src/jchuff.c \
    $$PWD/libjpeg/src/jcinit.c \
    $$PWD/libjpeg/src/jcmainct.c \
    $$PWD/libjpeg/src/jcmarker.c \
    $$PWD/libjpeg/src/jcmaster.c \
    $$PWD/libjpeg/src/jcomapi.c \
    $$PWD/libjpeg/src/jcparam.c \
    $$PWD/libjpeg/src/jcprepct.c \
    $$PWD/libjpeg/src/jcsample.c \
    $$PWD/libjpeg/src/jctrans.c \
    $$PWD/libjpeg/src/jdapimin.c \
    $$PWD/libjpeg/src/jdapistd.c \
    $$PWD/libjpeg/src/jdarith.c \
    $$PWD/libjpeg/src/jdatadst.c \
    $$PWD/libjpeg/src/jdatasrc.c \
    $$PWD/libjpeg/src/jdcoefct.c \
    $$PWD/libjpeg/src/jdcolor.c \
    $$PWD/libjpeg/src/jddctmgr.c \
    $$PWD/libjpeg/src/jdhuff.c \
    $$PWD/libjpeg/src/jdinput.c \
    $$PWD/libjpeg/src/jdmainct.c \
    $$PWD/libjpeg/src/jdmarker.c \
    $$PWD/libjpeg/src/jdmaster.c \
    $$PWD/libjpeg/src/jdmerge.c \
    $$PWD/libjpeg/src/jdpostct.c \
    $$PWD/libjpeg/src/jdsample.c \
    $$PWD/libjpeg/src/jdtrans.c \
    $$PWD/libjpeg/src/jerror.c \
    $$PWD/libjpeg/src/jfdctflt.c \
    $$PWD/libjpeg/src/jfdctfst.c \
    $$PWD/libjpeg/src/jfdctint.c \
    $$PWD/libjpeg/src/jidctflt.c \
    $$PWD/libjpeg/src/jidctfst.c \
    $$PWD/libjpeg/src/jidctint.c \
    $$PWD/libjpeg/src/jquant1.c \
    $$PWD/libjpeg/src/jquant2.c \
    $$PWD/libjpeg/src/jutils.c \
    $$PWD/libjpeg/src/jmemmgr.c \
    $$PWD/libjpeg/src/jsimd_none.c \
    $$PWD/libjpeg/src/jcphuff.c \
    $$PWD/libjpeg/src/jidctred.c \
    $$PWD/libjpeg/src/jdphuff.c \
    $$PWD/libjpeg/src/jmemnobs.c

TR_EXCLUDE += $$PWD/*
