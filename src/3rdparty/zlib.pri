INCLUDEPATH = $$PWD/zlib/src $$INCLUDEPATH
SOURCES+= \
    $$PWD/zlib/src/adler32.c \
    $$PWD/zlib/src/compress.c \
    $$PWD/zlib/src/crc32.c \
    $$PWD/zlib/src/deflate.c \
    $$PWD/zlib/src/gzclose.c \
    $$PWD/zlib/src/gzlib.c \
    $$PWD/zlib/src/gzread.c \
    $$PWD/zlib/src/gzwrite.c \
    $$PWD/zlib/src/infback.c \
    $$PWD/zlib/src/inffast.c \
    $$PWD/zlib/src/inflate.c \
    $$PWD/zlib/src/inftrees.c \
    $$PWD/zlib/src/trees.c \
    $$PWD/zlib/src/uncompr.c \
    $$PWD/zlib/src/zutil.c

TR_EXCLUDE += $$PWD/*
