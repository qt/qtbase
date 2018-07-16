# Qt core library codecs module

HEADERS += \
    codecs/qtextcodec_p.h \
    codecs/qutfcodec_p.h

SOURCES += \
    codecs/qutfcodec.cpp

qtConfig(textcodec) {
    HEADERS += \
        codecs/qlatincodec_p.h \
        codecs/qsimplecodec_p.h \
        codecs/qtextcodec.h

    SOURCES += \
        codecs/qlatincodec.cpp \
        codecs/qsimplecodec.cpp \
        codecs/qtextcodec.cpp

    qtConfig(codecs) {
        HEADERS += \
            codecs/qisciicodec_p.h \
            codecs/qtsciicodec_p.h

        SOURCES += \
            codecs/qisciicodec.cpp \
            codecs/qtsciicodec.cpp
    }

    qtConfig(icu) {
        HEADERS += \
            codecs/qicucodec_p.h
        SOURCES += \
            codecs/qicucodec.cpp
    } else {
        qtConfig(big_codecs) {
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
        }

        qtConfig(iconv) {
            HEADERS += codecs/qiconvcodec_p.h
            SOURCES += codecs/qiconvcodec.cpp
            QMAKE_USE_PRIVATE += iconv
        }

        win32 {
            SOURCES += codecs/qwindowscodec.cpp
            HEADERS += codecs/qwindowscodec_p.h
        }
    }
}
