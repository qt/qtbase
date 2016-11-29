# Qt core library codecs module

HEADERS += \
    codecs/qisciicodec_p.h \
    codecs/qlatincodec_p.h \
    codecs/qsimplecodec_p.h \
    codecs/qtextcodec_p.h \
    codecs/qtextcodec.h \
    codecs/qtsciicodec_p.h \
    codecs/qutfcodec_p.h

SOURCES += \
    codecs/qisciicodec.cpp \
    codecs/qlatincodec.cpp \
    codecs/qsimplecodec.cpp \
    codecs/qtextcodec.cpp \
    codecs/qtsciicodec.cpp \
    codecs/qutfcodec.cpp

qtConfig(icu) {
    HEADERS += \
        codecs/qicucodec_p.h
    SOURCES += \
        codecs/qicucodec.cpp
} else {
    HEADERS += \
        codecs/qgb18030codec_p.h \
        codecs/qeucjpcodec_p.h \
        codecs/qjiscodec_p.h \
        codecs/qsjiscodec_p.h \
        codecs/qeuckrcodec_p.h \
        codecs/qbig5codec_p.h

    SOURCES += \
        codecs/qgb18030codec.cpp \
        codecs/qjpunicode.cpp \
        codecs/qeucjpcodec.cpp \
        codecs/qjiscodec.cpp \
        codecs/qsjiscodec.cpp \
        codecs/qeuckrcodec.cpp \
        codecs/qbig5codec.cpp

    qtConfig(iconv) {
        HEADERS += codecs/qiconvcodec_p.h
        SOURCES += codecs/qiconvcodec.cpp
        qtConfig(gnu-libiconv): \
            QMAKE_USE_PRIVATE += iconv
    }

    win32 {
        SOURCES += codecs/qwindowscodec.cpp
        HEADERS += codecs/qwindowscodec_p.h
    }
}
