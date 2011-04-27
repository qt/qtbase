DEFINES += MNG_BUILD_SO
DEFINES += MNG_NO_INCLUDE_JNG
INCLUDEPATH += $$PWD/libmng
SOURCES += \
    $$PWD/libmng/libmng_callback_xs.c \
    $$PWD/libmng/libmng_chunk_io.c \
    $$PWD/libmng/libmng_chunk_descr.c \
    $$PWD/libmng/libmng_chunk_prc.c \
    $$PWD/libmng/libmng_chunk_xs.c \
    $$PWD/libmng/libmng_cms.c \
    $$PWD/libmng/libmng_display.c \
    $$PWD/libmng/libmng_dither.c \
    $$PWD/libmng/libmng_error.c \
    $$PWD/libmng/libmng_filter.c \
    $$PWD/libmng/libmng_hlapi.c \
    $$PWD/libmng/libmng_jpeg.c \
    $$PWD/libmng/libmng_object_prc.c \
    $$PWD/libmng/libmng_pixels.c \
    $$PWD/libmng/libmng_prop_xs.c \
    $$PWD/libmng/libmng_read.c \
    $$PWD/libmng/libmng_trace.c \
    $$PWD/libmng/libmng_write.c \
    $$PWD/libmng/libmng_zlib.c

include($$PWD/zlib_dependency.pri)
