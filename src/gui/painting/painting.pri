# Qt gui library, paint module

HEADERS += \
        painting/qbackingstore.h \
        painting/qbezier_p.h \
        painting/qblendfunctions_p.h \
        painting/qblittable_p.h \
        painting/qbrush.h \
        painting/qcolor.h \
        painting/qcolor_p.h \
        painting/qcolorprofile_p.h \
        painting/qcosmeticstroker_p.h \
        painting/qdatabuffer_p.h \
        painting/qdrawhelper_p.h \
        painting/qdrawhelper_x86_p.h \
        painting/qdrawingprimitive_sse2_p.h \
        painting/qemulationpaintengine_p.h \
        painting/qfixed_p.h \
        painting/qgrayraster_p.h \
        painting/qmatrix.h \
        painting/qmemrotate_p.h \
        painting/qoutlinemapper_p.h \
        painting/qpagedpaintdevice.h \
        painting/qpagedpaintdevice_p.h \
        painting/qpagelayout.h \
        painting/qpagesize.h \
        painting/qpaintdevice.h \
        painting/qpaintengine.h \
        painting/qpaintengine_p.h \
        painting/qpaintengineex_p.h \
        painting/qpaintengine_blitter_p.h \
        painting/qpaintengine_raster_p.h \
        painting/qpainter.h \
        painting/qpainter_p.h \
        painting/qpainterpath.h \
        painting/qpainterpath_p.h \
        painting/qvectorpath_p.h \
        painting/qpathclipper_p.h \
        painting/qpdf_p.h \
        painting/qpdfwriter.h \
        painting/qpen.h \
        painting/qpolygon.h \
        painting/qpolygonclipper_p.h \
        painting/qrasterdefs_p.h \
        painting/qrasterizer_p.h \
        painting/qrbtree_p.h \
        painting/qregion.h \
        painting/qrgb.h \
        painting/qrgba64.h \
        painting/qrgba64_p.h \
        painting/qstroker_p.h \
        painting/qtextureglyphcache_p.h \
        painting/qtransform.h \
        painting/qtriangulatingstroker_p.h \
        painting/qtriangulator_p.h \
        painting/qplatformbackingstore.h \
        painting/qpathsimplifier_p.h


SOURCES += \
        painting/qbackingstore.cpp \
        painting/qbezier.cpp \
        painting/qblendfunctions.cpp \
        painting/qblittable.cpp \
        painting/qbrush.cpp \
        painting/qcolor.cpp \
        painting/qcolorprofile.cpp \
        painting/qcompositionfunctions.cpp \
        painting/qcosmeticstroker.cpp \
        painting/qdrawhelper.cpp \
        painting/qemulationpaintengine.cpp \
        painting/qgrayraster.c \
        painting/qimagescale.cpp \
        painting/qmatrix.cpp \
        painting/qmemrotate.cpp \
        painting/qoutlinemapper.cpp \
        painting/qpagedpaintdevice.cpp \
        painting/qpagelayout.cpp \
        painting/qpagesize.cpp \
        painting/qpaintdevice.cpp \
        painting/qpaintengine.cpp \
        painting/qpaintengineex.cpp \
        painting/qpaintengine_blitter.cpp \
        painting/qpaintengine_raster.cpp \
        painting/qpainter.cpp \
        painting/qpainterpath.cpp \
        painting/qpathclipper.cpp \
        painting/qpdf.cpp \
        painting/qpdfwriter.cpp \
        painting/qpen.cpp \
        painting/qpolygon.cpp \
        painting/qrasterizer.cpp \
        painting/qregion.cpp \
        painting/qstroker.cpp \
        painting/qtextureglyphcache.cpp \
        painting/qtransform.cpp \
        painting/qtriangulatingstroker.cpp \
        painting/qtriangulator.cpp \
        painting/qplatformbackingstore.cpp \
        painting/qpathsimplifier.cpp

RESOURCES += \
        painting/qpdf.qrc

darwin {
    HEADERS += painting/qcoregraphics_p.h
    SOURCES += painting/qcoregraphics.mm
}

qtConfig(cssparser) {
    SOURCES += \
        painting/qcssutil.cpp
}

# Causes internal compiler errors with at least GCC 5.3.1:
gcc:equals(QT_GCC_MAJOR_VERSION, 5) {
    SOURCES -= painting/qdrawhelper.cpp
    NO_PCH_SOURCES += painting/qdrawhelper.cpp
}

SSE2_SOURCES += painting/qdrawhelper_sse2.cpp
SSSE3_SOURCES += painting/qdrawhelper_ssse3.cpp
SSE4_1_SOURCES += painting/qdrawhelper_sse4.cpp \
                  painting/qimagescale_sse4.cpp
AVX2_SOURCES += painting/qdrawhelper_avx2.cpp

NEON_SOURCES += painting/qdrawhelper_neon.cpp painting/qimagescale_neon.cpp
NEON_HEADERS += painting/qdrawhelper_neon_p.h
NEON_ASM += ../3rdparty/pixman/pixman-arm-neon-asm.S painting/qdrawhelper_neon_asm.S
!uikit:!win32:contains(QT_ARCH, "arm"): CONFIG += no_clang_integrated_as
!uikit:!win32:!contains(QT_ARCH, "arm64"): DEFINES += ENABLE_PIXMAN_DRAWHELPERS

MIPS_DSP_SOURCES += painting/qdrawhelper_mips_dsp.cpp
MIPS_DSP_HEADERS += painting/qdrawhelper_mips_dsp_p.h painting/qt_mips_asm_dsp_p.h
MIPS_DSP_ASM += painting/qdrawhelper_mips_dsp_asm.S
MIPS_DSPR2_ASM += painting/qdrawhelper_mips_dspr2_asm.S

include($$PWD/../../3rdparty/zlib_dependency.pri)
