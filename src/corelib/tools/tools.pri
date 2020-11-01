# Qt tools module

intel_icc: QMAKE_CXXFLAGS += -fp-model strict

HEADERS +=  \
        tools/qalgorithms.h \
        tools/qarraydata.h \
        tools/qarraydataops.h \
        tools/qarraydatapointer.h \
        tools/qbitarray.h \
        tools/qcache.h \
        tools/qcontainerfwd.h \
        tools/qcontainertools_impl.h \
        tools/qcryptographichash.h \
        tools/qduplicatetracker_p.h \
        tools/qflatmap_p.h \
        tools/qfreelist_p.h \
        tools/qhash.h \
        tools/qhashfunctions.h \
        tools/qiterator.h \
        tools/qline.h \
        tools/qlist.h \
        tools/qmakearray_p.h \
        tools/qmap.h \
        tools/qmargins.h \
        tools/qmessageauthenticationcode.h \
        tools/qcontiguouscache.h \
        tools/qoffsetstringarray_p.h \
        tools/qpair.h \
        tools/qpoint.h \
        tools/qqueue.h \
        tools/qrect.h \
        tools/qringbuffer_p.h \
        tools/qrefcount.h \
        tools/qscopeguard.h \
        tools/qscopedpointer.h \
        tools/qscopedvaluerollback.h \
        tools/qshareddata.h \
        tools/qshareddata_impl.h \
        tools/qsharedpointer.h \
        tools/qsharedpointer_impl.h \
        tools/qset.h \
        tools/qsize.h \
        tools/qstack.h \
        tools/qtools_p.h \
        tools/qtaggedpointer.h \
        tools/qvarlengtharray.h \
        tools/qvector.h \
        tools/qversionnumber.h

SOURCES += \
        tools/qarraydata.cpp \
        tools/qbitarray.cpp \
        tools/qcryptographichash.cpp \
        tools/qfreelist.cpp \
        tools/qhash.cpp \
        tools/qline.cpp \
        tools/qpoint.cpp \
        tools/qmargins.cpp \
        tools/qmessageauthenticationcode.cpp \
        tools/qcontiguouscache.cpp \
        tools/qrect.cpp \
        tools/qrefcount.cpp \
        tools/qringbuffer.cpp \
        tools/qshareddata.cpp \
        tools/qsharedpointer.cpp \
        tools/qsize.cpp \
        tools/qversionnumber.cpp

qtConfig(system-zlib) {
    include($$PWD/../../3rdparty/zlib_dependency.pri)
} else {
    CONFIG += no_core_dep
    include($$PWD/../../3rdparty/zlib.pri)
}

qtConfig(commandlineparser) {
    HEADERS += \
        tools/qcommandlineoption.h \
        tools/qcommandlineparser.h
    SOURCES += \
        tools/qcommandlineoption.cpp \
        tools/qcommandlineparser.cpp
}

INCLUDEPATH += ../3rdparty/md5 \
               ../3rdparty/md4 \
               ../3rdparty/sha3

qtConfig(system-doubleconversion) {
    QMAKE_USE_PRIVATE += doubleconversion
} else: qtConfig(doubleconversion) {
    include($$PWD/../../3rdparty/double-conversion/double-conversion.pri)
}

qtConfig(system-libb2) {
    QMAKE_USE_PRIVATE += libb2
}

qtConfig(easingcurve) {
    HEADERS += \
        tools/qeasingcurve.h \
        tools/qtimeline.h

    SOURCES += \
        tools/qeasingcurve.cpp \
        tools/qtimeline.cpp
}

# Note: libm should be present by default becaue this is C++
unix:!macx-icc:!vxworks:!haiku:!integrity:!wasm: LIBS_PRIVATE += -lm

TR_EXCLUDE += ../3rdparty/*

# MIPS DSP
MIPS_DSP_HEADERS += ../gui/painting/qt_mips_asm_dsp_p.h
