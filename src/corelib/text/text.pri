# Qt text / string / character / unicode / byte array module

HEADERS +=  \
        text/qbytearray.h \
        text/qbytearray_p.h \
        text/qbytearraylist.h \
        text/qbytearraymatcher.h \
        text/qbytedata_p.h \
        text/qchar.h \
        text/qcollator.h \
        text/qcollator_p.h \
        text/qdoublescanprint_p.h \
        text/qlocale.h \
        text/qlocale_p.h \
        text/qlocale_tools_p.h \
        text/qlocale_data_p.h \
        text/qregexp.h \
        text/qstring.h \
        text/qstringalgorithms.h \
        text/qstringalgorithms_p.h \
        text/qstringbuilder.h \
        text/qstringiterator_p.h \
        text/qstringlist.h \
        text/qstringliteral.h \
        text/qstringmatcher.h \
        text/qstringview.h \
        text/qtextboundaryfinder.h \
        text/qunicodetables_p.h \
        text/qunicodetools_p.h


SOURCES += \
        text/qbytearray.cpp \
        text/qbytearraylist.cpp \
        text/qbytearraymatcher.cpp \
        text/qcollator.cpp \
        text/qlocale.cpp \
        text/qlocale_tools.cpp \
        text/qregexp.cpp \
        text/qstring.cpp \
        text/qstringbuilder.cpp \
        text/qstringlist.cpp \
        text/qstringview.cpp \
        text/qtextboundaryfinder.cpp \
        text/qunicodetools.cpp \
        text/qvsnprintf.cpp

NO_PCH_SOURCES += text/qstring_compat.cpp
false: SOURCES += $$NO_PCH_SOURCES # Hack for QtCreator

!nacl:darwin: {
    SOURCES += text/qlocale_mac.mm
}
else:unix {
    SOURCES += text/qlocale_unix.cpp
}
else:win32 {
    SOURCES += text/qlocale_win.cpp
} else:integrity {
    SOURCES += text/qlocale_unix.cpp
}

qtConfig(icu) {
    QMAKE_USE_PRIVATE += icu

    SOURCES += text/qlocale_icu.cpp \
               text/qcollator_icu.cpp
} else: win32 {
    SOURCES += text/qcollator_win.cpp
} else: macos {
    SOURCES += text/qcollator_macx.cpp
} else {
    SOURCES += text/qcollator_posix.cpp
}

qtConfig(regularexpression) {
    QMAKE_USE_PRIVATE += pcre2

    HEADERS += \
        text/qregularexpression.h
    SOURCES += text/qregularexpression.cpp
}

INCLUDEPATH += ../3rdparty/harfbuzz/src
HEADERS += ../3rdparty/harfbuzz/src/harfbuzz.h
SOURCES += ../3rdparty/harfbuzz/src/harfbuzz-buffer.c \
           ../3rdparty/harfbuzz/src/harfbuzz-gdef.c \
           ../3rdparty/harfbuzz/src/harfbuzz-gsub.c \
           ../3rdparty/harfbuzz/src/harfbuzz-gpos.c \
           ../3rdparty/harfbuzz/src/harfbuzz-impl.c \
           ../3rdparty/harfbuzz/src/harfbuzz-open.c \
           ../3rdparty/harfbuzz/src/harfbuzz-stream.c \
           ../3rdparty/harfbuzz/src/harfbuzz-shaper-all.cpp \
           text/qharfbuzz.cpp
HEADERS += text/qharfbuzz_p.h

TR_EXCLUDE += ../3rdparty/*

# MIPS DSP
MIPS_DSP_ASM += text/qstring_mips_dsp_asm.S
