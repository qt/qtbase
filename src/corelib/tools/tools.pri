# Qt tools module

intel_icc: QMAKE_CXXFLAGS += -fp-model strict

HEADERS +=  \
        tools/qalgorithms.h \
        tools/qarraydata.h \
        tools/qarraydataops.h \
        tools/qarraydatapointer.h \
        tools/qbitarray.h \
        tools/qbytearray.h \
        tools/qbytearray_p.h \
        tools/qbytearraylist.h \
        tools/qbytearraymatcher.h \
        tools/qbytedata_p.h \
        tools/qcache.h \
        tools/qchar.h \
        tools/qcollator.h \
        tools/qcollator_p.h \
        tools/qcontainerfwd.h \
        tools/qcryptographichash.h \
        tools/qdatetime.h \
        tools/qdatetime_p.h \
        tools/qdoublescanprint_p.h \
        tools/qeasingcurve.h \
        tools/qfreelist_p.h \
        tools/qhash.h \
        tools/qhashfunctions.h \
        tools/qiterator.h \
        tools/qline.h \
        tools/qlinkedlist.h \
        tools/qlist.h \
        tools/qlocale.h \
        tools/qlocale_p.h \
        tools/qlocale_tools_p.h \
        tools/qlocale_data_p.h \
        tools/qmap.h \
        tools/qmargins.h \
        tools/qmessageauthenticationcode.h \
        tools/qcontiguouscache.h \
        tools/qpair.h \
        tools/qpoint.h \
        tools/qqueue.h \
        tools/qrect.h \
        tools/qregexp.h \
        tools/qringbuffer_p.h \
        tools/qrefcount.h \
        tools/qscopedpointer.h \
        tools/qscopedpointer_p.h \
        tools/qscopedvaluerollback.h \
        tools/qshareddata.h \
        tools/qsharedpointer.h \
        tools/qsharedpointer_impl.h \
        tools/qset.h \
        tools/qsimd_p.h \
        tools/qsize.h \
        tools/qstack.h \
        tools/qstring.h \
        tools/qstringalgorithms.h \
        tools/qstringalgorithms_p.h \
        tools/qstringbuilder.h \
        tools/qstringiterator_p.h \
        tools/qstringlist.h \
        tools/qstringliteral.h \
        tools/qstringmatcher.h \
        tools/qstringview.h \
        tools/qtextboundaryfinder.h \
        tools/qtimeline.h \
        tools/qtools_p.h \
        tools/qunicodetables_p.h \
        tools/qunicodetools_p.h \
        tools/qvarlengtharray.h \
        tools/qvector.h \
        tools/qversionnumber.h


SOURCES += \
        tools/qarraydata.cpp \
        tools/qbitarray.cpp \
        tools/qbytearray.cpp \
        tools/qbytearraylist.cpp \
        tools/qbytearraymatcher.cpp \
        tools/qcollator.cpp \
        tools/qcryptographichash.cpp \
        tools/qdatetime.cpp \
        tools/qeasingcurve.cpp \
        tools/qfreelist.cpp \
        tools/qhash.cpp \
        tools/qline.cpp \
        tools/qlinkedlist.cpp \
        tools/qlist.cpp \
        tools/qlocale.cpp \
        tools/qlocale_tools.cpp \
        tools/qpoint.cpp \
        tools/qmap.cpp \
        tools/qmargins.cpp \
        tools/qmessageauthenticationcode.cpp \
        tools/qcontiguouscache.cpp \
        tools/qrect.cpp \
        tools/qregexp.cpp \
        tools/qrefcount.cpp \
        tools/qringbuffer.cpp \
        tools/qshareddata.cpp \
        tools/qsharedpointer.cpp \
        tools/qsimd.cpp \
        tools/qsize.cpp \
        tools/qstring.cpp \
        tools/qstringbuilder.cpp \
        tools/qstringlist.cpp \
        tools/qstringview.cpp \
        tools/qtextboundaryfinder.cpp \
        tools/qtimeline.cpp \
        tools/qunicodetools.cpp \
        tools/qvsnprintf.cpp \
        tools/qversionnumber.cpp

NO_PCH_SOURCES = tools/qstring_compat.cpp
msvc: NO_PCH_SOURCES += tools/qvector_msvc.cpp
false: SOURCES += $$NO_PCH_SOURCES # Hack for QtCreator

!nacl:mac: {
    SOURCES += tools/qlocale_mac.mm
}
else:unix {
    SOURCES += tools/qlocale_unix.cpp
}
else:win32 {
    SOURCES += tools/qlocale_win.cpp
    winrt-*-msvc2013: LIBS += advapi32.lib
} else:integrity {
    SOURCES += tools/qlocale_unix.cpp
}

qtConfig(system-zlib) {
    include($$PWD/../../3rdparty/zlib_dependency.pri)
} else {
    CONFIG += no_core_dep
    include($$PWD/../../3rdparty/zlib.pri)
}

qtConfig(icu) {
    QMAKE_USE_PRIVATE += icu

    SOURCES += tools/qlocale_icu.cpp \
               tools/qcollator_icu.cpp
} else: win32 {
    SOURCES += tools/qcollator_win.cpp
} else: macx {
    SOURCES += tools/qcollator_macx.cpp
} else {
    SOURCES += tools/qcollator_posix.cpp
}

qtConfig(timezone) {
    HEADERS += \
        tools/qtimezone.h \
        tools/qtimezoneprivate_p.h \
        tools/qtimezoneprivate_data_p.h
    SOURCES += \
        tools/qtimezone.cpp \
        tools/qtimezoneprivate.cpp
    !nacl:darwin: \
        SOURCES += tools/qtimezoneprivate_mac.mm
    else: android:!android-embedded: \
        SOURCES += tools/qtimezoneprivate_android.cpp
    else: unix: \
        SOURCES += tools/qtimezoneprivate_tz.cpp
    else: win32: \
        SOURCES += tools/qtimezoneprivate_win.cpp
    qtConfig(icu): \
        SOURCES += tools/qtimezoneprivate_icu.cpp
}

qtConfig(datetimeparser) {
    HEADERS += tools/qdatetimeparser_p.h
    SOURCES += tools/qdatetimeparser.cpp
}

qtConfig(regularexpression) {
    QMAKE_USE_PRIVATE += pcre2

    HEADERS += tools/qregularexpression.h
    SOURCES += tools/qregularexpression.cpp
}

qtConfig(commandlineparser) {
    HEADERS += \
        tools/qcommandlineoption.h \
        tools/qcommandlineparser.h
    SOURCES += \
        tools/qcommandlineoption.cpp \
        tools/qcommandlineparser.cpp
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
           tools/qharfbuzz.cpp
HEADERS += tools/qharfbuzz_p.h

INCLUDEPATH += ../3rdparty/md5 \
               ../3rdparty/md4 \
               ../3rdparty/sha3

qtConfig(system-doubleconversion) {
    QMAKE_USE_PRIVATE += doubleconversion
} else: qtConfig(doubleconversion) {
    include($$PWD/../../3rdparty/double-conversion/double-conversion.pri)
}

# Note: libm should be present by default becaue this is C++
unix:!macx-icc:!vxworks:!haiku:!integrity: LIBS_PRIVATE += -lm

TR_EXCLUDE += ../3rdparty/*

# MIPS DSP
MIPS_DSP_ASM += tools/qstring_mips_dsp_asm.S
MIPS_DSP_HEADERS += ../gui/painting/qt_mips_asm_dsp_p.h
