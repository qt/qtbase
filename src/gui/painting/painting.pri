# Qt gui library, paint module

HEADERS += \
        painting/qbackingstore.h \
        painting/qbezier_p.h \
        painting/qbrush.h \
        painting/qcolor.h \
        painting/qcolor_p.h \
        painting/qcosmeticstroker_p.h \
        painting/qemulationpaintengine_p.h \
        painting/qmatrix.h \
        painting/qmemrotate_p.h \
        painting/qoutlinemapper_p.h \
        painting/qpagedpaintdevice.h \
        painting/qpagedpaintdevice_p.h \
        painting/qpaintdevice.h \
        painting/qpaintengine.h \
        painting/qpaintengine_p.h \
        painting/qpaintengineex_p.h \
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
        painting/qrasterizer_p.h \
        painting/qregion.h \
        painting/qstroker_p.h \
        painting/qtessellator_p.h \
        painting/qtextureglyphcache_p.h \
        painting/qtransform.h \
        painting/qplatformbackingstore_qpa.h \
        painting/qpaintbuffer_p.h


SOURCES += \
        painting/qbackingstore.cpp \
        painting/qbezier.cpp \
        painting/qblendfunctions.cpp \
        painting/qbrush.cpp \
        painting/qcolor.cpp \
        painting/qcolor_p.cpp \
        painting/qcosmeticstroker.cpp \
        painting/qcssutil.cpp \
        painting/qemulationpaintengine.cpp \
        painting/qmatrix.cpp \
        painting/qmemrotate.cpp \
        painting/qoutlinemapper.cpp \
        painting/qpagedpaintdevice.cpp \
        painting/qpaintdevice.cpp \
        painting/qpaintdevice_qpa.cpp \
        painting/qpaintengine.cpp \
        painting/qpaintengineex.cpp \
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
        painting/qtessellator.cpp \
        painting/qtextureglyphcache.cpp \
        painting/qtransform.cpp \
        painting/qplatformbackingstore_qpa.cpp \
        painting/qpaintbuffer.cpp

        SOURCES +=                                      \
                painting/qpaintengine_raster.cpp        \
                painting/qdrawhelper.cpp                \
                painting/qimagescale.cpp                \
                painting/qgrayraster.c                  \
                painting/qpaintengine_blitter.cpp       \
                painting/qblittable.cpp                 \

        HEADERS +=                                      \
                painting/qpaintengine_raster_p.h        \
                painting/qdrawhelper_p.h                \
                painting/qblendfunctions_p.h            \
                painting/qrasterdefs_p.h                \
                painting/qgrayraster_p.h                \
                painting/qpaintengine_blitter_p.h       \
                painting/qblittable_p.h                 \



if(mmx|3dnow|sse|sse2|iwmmxt) {
    HEADERS += painting/qdrawhelper_x86_p.h \
               painting/qdrawhelper_mmx_p.h \
               painting/qdrawhelper_sse_p.h \
               painting/qdrawingprimitive_sse2_p.h
    MMX_SOURCES += painting/qdrawhelper_mmx.cpp
    MMX3DNOW_SOURCES += painting/qdrawhelper_mmx3dnow.cpp
    SSE3DNOW_SOURCES += painting/qdrawhelper_sse3dnow.cpp
    SSE_SOURCES += painting/qdrawhelper_sse.cpp
    SSE2_SOURCES += painting/qdrawhelper_sse2.cpp
    SSSE3_SOURCES += painting/qdrawhelper_ssse3.cpp
    IWMMXT_SOURCES += painting/qdrawhelper_iwmmxt.cpp
}

symbian {
        HEADERS += painting/qdrawhelper_arm_simd_p.h
        armccIfdefBlock = \
        "$${LITERAL_HASH}if defined(ARMV6)" \
        "MACRO QT_HAVE_ARM_SIMD" \
        "SOURCEPATH 	painting" \
        "SOURCE			qdrawhelper_arm_simd.cpp" \
        "$${LITERAL_HASH}endif"

        MMP_RULES += armccIfdefBlock
        QMAKE_CXXFLAGS.ARMCC *= -O3
}

NEON_SOURCES += painting/qdrawhelper_neon.cpp
NEON_HEADERS += painting/qdrawhelper_neon_p.h
NEON_ASM += ../3rdparty/pixman/pixman-arm-neon-asm.S painting/qdrawhelper_neon_asm.S

include($$PWD/../../3rdparty/zlib_dependency.pri)
