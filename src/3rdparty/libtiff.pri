INCLUDEPATH += $$PWD/libtiff/libtiff
SOURCES += \
    $$PWD/libtiff/libtiff/tif_aux.c \
    $$PWD/libtiff/libtiff/tif_close.c \
    $$PWD/libtiff/libtiff/tif_codec.c \
    $$PWD/libtiff/libtiff/tif_color.c \
    $$PWD/libtiff/libtiff/tif_compress.c \
    $$PWD/libtiff/libtiff/tif_dir.c \
    $$PWD/libtiff/libtiff/tif_dirinfo.c \
    $$PWD/libtiff/libtiff/tif_dirread.c \
    $$PWD/libtiff/libtiff/tif_dirwrite.c \
    $$PWD/libtiff/libtiff/tif_dumpmode.c \
    $$PWD/libtiff/libtiff/tif_error.c \
    $$PWD/libtiff/libtiff/tif_extension.c \
    $$PWD/libtiff/libtiff/tif_fax3.c \
    $$PWD/libtiff/libtiff/tif_fax3sm.c \
    $$PWD/libtiff/libtiff/tif_flush.c \
    $$PWD/libtiff/libtiff/tif_getimage.c \
    $$PWD/libtiff/libtiff/tif_luv.c \
    $$PWD/libtiff/libtiff/tif_lzw.c \
    $$PWD/libtiff/libtiff/tif_next.c \
    $$PWD/libtiff/libtiff/tif_open.c \
    $$PWD/libtiff/libtiff/tif_packbits.c \
    $$PWD/libtiff/libtiff/tif_pixarlog.c \
    $$PWD/libtiff/libtiff/tif_predict.c \
    $$PWD/libtiff/libtiff/tif_print.c \
    $$PWD/libtiff/libtiff/tif_read.c \
    $$PWD/libtiff/libtiff/tif_strip.c \
    $$PWD/libtiff/libtiff/tif_swab.c \
    $$PWD/libtiff/libtiff/tif_thunder.c \
    $$PWD/libtiff/libtiff/tif_tile.c \
    $$PWD/libtiff/libtiff/tif_version.c \
    $$PWD/libtiff/libtiff/tif_warning.c \
    $$PWD/libtiff/libtiff/tif_write.c \
    $$PWD/libtiff/libtiff/tif_zip.c

wince*:       SOURCES += $$PWD/../corelib/kernel/qfunctions_wince.cpp \
                         $$PWD/libtiff/libtiff/tif_wince.c
win32:        SOURCES += $$PWD/libtiff/libtiff/tif_win32.c
else:         SOURCES += $$PWD/libtiff/libtiff/tif_unix.c
symbian:      SOURCES += $$PWD/libtiff/port/lfind.c

include($$PWD/zlib_dependency.pri)
