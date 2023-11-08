TARGET = qtharfbuzz

CONFIG += \
    static \
    hide_symbols \
    exceptions_off rtti_off warn_off

MODULE_INCLUDEPATH += $$PWD/include


# built-in shapers list configuration:
SHAPERS += opentype       # HB's main shaper; enabling it should be enough most of the time

# native shaper on Apple platforms; could be used alone to handle both OT and AAT fonts
darwin: SHAPERS += coretext

# fallback shaper: not really useful with opentype or coretext shaper but needed for linking
SHAPERS += fallback

DEFINES += HAVE_CONFIG_H
DEFINES += HB_NO_UNICODE_FUNCS
DEFINES += HB_NDEBUG
DEFINES += HB_EXTERN=

# platform/compiler specific definitions
DEFINES += HAVE_ATEXIT
unix: DEFINES += HAVE_PTHREAD HAVE_SCHED_H HAVE_SCHED_YIELD
win32: DEFINES += HB_NO_WIN1256
msvc:winrt: QMAKE_CXXFLAGS += /bigobj    # prevent error C1128

#Workaround https://code.google.com/p/android/issues/detail?id=194631
android: DEFINES += _POSIX_C_SOURCE=200112L

# Harfbuzz-NG inside Qt uses the Qt atomics (inline code only)
INCLUDEPATH += $$QT.core.includes
DEFINES += QT_NO_VERSION_TAGGING

SOURCES += \
    $$PWD/hb-dummy.cc \
    $$PWD/src/hb-aat-layout.cc \
    $$PWD/src/hb-aat-map.cc \
    $$PWD/src/hb-blob.cc \
    $$PWD/src/hb-buffer.cc \
    $$PWD/src/hb-buffer-serialize.cc \
    $$PWD/src/hb-common.cc \
    $$PWD/src/hb-draw.cc \
    $$PWD/src/hb-face.cc \
    $$PWD/src/hb-face-builder.cc \
    $$PWD/src/hb-font.cc \
    $$PWD/src/hb-map.cc \
    $$PWD/src/hb-number.cc \
    $$PWD/src/hb-ot-tag.cc \
    $$PWD/src/hb-outline.cc \
    $$PWD/src/hb-paint.cc \
    $$PWD/src/hb-paint-extents.cc \
    $$PWD/src/hb-set.cc \
    $$PWD/src/hb-shape.cc \
    $$PWD/src/hb-shape-plan.cc \
    $$PWD/src/hb-shaper.cc \
    $$PWD/src/hb-style.cc \
    $$PWD/src/hb-subset.cc \
    $$PWD/src/hb-subset-cff-common.cc \
    $$PWD/src/hb-subset-cff1.cc \
    $$PWD/src/hb-subset-cff2.cc \
    $$PWD/src/hb-subset-input.cc \
    $$PWD/src/hb-subset-instancer-solver.cc \
    $$PWD/src/hb-subset-plan.cc \
    $$PWD/src/hb-subset-repacker.cc \
    $$PWD/src/hb-unicode.cc \
    $$PWD/src/hb-buffer-verify.cc

HEADERS += \
    $$PWD/src/hb-buffer-deserialize-json.hh \
    $$PWD/src/hb-buffer-deserialize-text-glyphs.hh \
    $$PWD/src/hb-buffer-deserialize-text-unicode.hh \
    $$PWD/src/hb-debug.hh \
    $$PWD/src/hb-face.hh \
    $$PWD/src/hb-mutex.hh \
    $$PWD/src/hb-ot-cmap-table.hh \
    $$PWD/src/hb-ot-glyf-table.hh \
    $$PWD/src/hb-ot-head-table.hh \
    $$PWD/src/hb-ot-hhea-table.hh \
    $$PWD/src/hb-ot-hmtx-table.hh \
    $$PWD/src/hb-ot-maxp-table.hh \
    $$PWD/src/hb-ot-name-table.hh \
    $$PWD/src/hb-ot-os2-table.hh \
    $$PWD/src/hb-ot-post-table.hh \
    $$PWD/src/hb-set.hh \
    $$PWD/src/hb-set-digest.hh \
    $$PWD/src/hb-shape-plan.hh \
    $$PWD/src/hb-shaper.hh \
    $$PWD/src/hb-shaper-impl.hh \
    $$PWD/src/hb-shaper-list.hh \
    $$PWD/src/hb-string-array.hh \
    $$PWD/src/hb-subset-plan-member-list.hh \
    $$PWD/src/hb-subset-repacker.h \
    $$PWD/src/hb-unicode.hh

HEADERS += \
    $$PWD/src/hb.h \
    $$PWD/src/hb-algs.hh \
    $$PWD/src/hb-atomic.hh \
    $$PWD/src/hb-blob.h \
    $$PWD/src/hb-buffer.h \
    $$PWD/src/hb-buffer.hh \
    $$PWD/src/hb-cache.hh \
    $$PWD/src/hb-common.h \
    $$PWD/src/hb-deprecated.h \
    $$PWD/src/hb-draw.h \
    $$PWD/src/hb-draw.hh \
    $$PWD/src/hb-face.h \
    $$PWD/src/hb-font.h \
    $$PWD/src/hb-font.hh \
    $$PWD/src/hb-ft-colr.hh \
    $$PWD/src/hb-limits.hh \
    $$PWD/src/hb-map.h \
    $$PWD/src/hb-map.hh \
    $$PWD/src/hb-object.hh \
    $$PWD/src/hb-open-file.hh \
    $$PWD/src/hb-open-type.hh \
    $$PWD/src/hb-ot-cff1-std-str.hh \
    $$PWD/src/hb-outline.hh \
    $$PWD/src/hb-paint.h \
    $$PWD/src/hb-paint.hh \
    $$PWD/src/hb-paint-extents.hh \
    $$PWD/src/hb-priority-queue.hh \
    $$PWD/src/hb-repacker.hh \
    $$PWD/src/hb-set.h \
    $$PWD/src/hb-shape.h \
    $$PWD/src/hb-shape-plan.h \
    $$PWD/src/hb-style.h \
    $$PWD/src/hb-unicode.h \
    $$PWD/src/hb-utf.hh \
    $$PWD/src/hb-version.h

contains(SHAPERS, opentype) {
    DEFINES += HAVE_OT

    SOURCES += \
        $$PWD/src/hb-ot-cff1-table.cc \
        $$PWD/src/hb-ot-cff2-table.cc \
        $$PWD/src/hb-ot-color.cc \
        $$PWD/src/hb-ot-face.cc \
        $$PWD/src/hb-ot-font.cc \
        $$PWD/src/hb-ot-layout.cc \
        $$PWD/src/hb-ot-map.cc \
        $$PWD/src/hb-ot-math.cc \
        $$PWD/src/hb-ot-meta.cc \
        $$PWD/src/hb-ot-metrics.cc \
        $$PWD/src/hb-ot-name.cc \
        $$PWD/src/hb-ot-shape.cc \
        $$PWD/src/hb-ot-shaper-arabic.cc \
        $$PWD/src/hb-ot-shaper-default.cc \
        $$PWD/src/hb-ot-shaper-hangul.cc \
        $$PWD/src/hb-ot-shaper-hebrew.cc \
        $$PWD/src/hb-ot-shaper-indic.cc \
        $$PWD/src/hb-ot-shaper-thai.cc \
        $$PWD/src/hb-ot-shape-fallback.cc \
        $$PWD/src/hb-ot-shape-normalize.cc \
        $$PWD/src/hb-ot-shaper-indic-table.cc \
        $$PWD/src/hb-ot-shaper-khmer.cc \
        $$PWD/src/hb-ot-shaper-myanmar.cc \
        $$PWD/src/hb-ot-shaper-syllabic.cc \
        $$PWD/src/hb-ot-shaper-use.cc \
        $$PWD/src/hb-ot-shaper-vowel-constraints.cc \
        $$PWD/src/hb-ot-var.cc

    HEADERS += \
        $$PWD/src/hb-ot-kern-table.hh \
        $$PWD/src/hb-ot-layout-gdef-table.hh \
        $$PWD/src/hb-ot-layout-gpos-table.hh \
        $$PWD/src/hb-ot-layout-gsub-table.hh \
        $$PWD/src/hb-ot-layout-jstf-table.hh \
        $$PWD/src/hb-ot-layout.hh \
        $$PWD/src/hb-ot-map.hh \
        $$PWD/src/hb-ot-math-table.hh \
        $$PWD/src/hb-ot-post-macroman.hh \
        $$PWD/src/hb-ot-post-table-v2subset.hh \
        $$PWD/src/hb-ot-shaper-arabic-joining-list.hh \
        $$PWD/src/hb-ot-shaper-arabic-pua.hh \
        $$PWD/src/hb-ot-shaper-indic-machine.hh \
        $$PWD/src/hb-ot-shaper-arabic-fallback.hh \
        $$PWD/src/hb-ot-shaper-arabic.hh \
        $$PWD/src/hb-ot-shaper-arabic-table.hh \
#        $$PWD/src/hb-ot-shaper-arabic-win1256.hh \ # disabled with HB_NO_WIN1256
        $$PWD/src/hb-ot-shaper.hh \
        $$PWD/src/hb-ot-shape-fallback.hh \
        $$PWD/src/hb-ot-shape-normalize.hh \
        $$PWD/src/hb-ot-shaper-indic.hh \
        $$PWD/src/hb-ot-shaper-khmer-machine.hh \
        $$PWD/src/hb-ot-shaper-myanmar-machine.hh \
        $$PWD/src/hb-ot-shaper-syllabic.hh \
        $$PWD/src/hb-ot-shaper-use-machine.hh \
        $$PWD/src/hb-ot-shaper-use-table.hh \
        $$PWD/src/hb-ot-shaper-vowel-constraints.hh \
        $$PWD/src/hb-subset-accelerator.hh \
        $$PWD/src/hb-ot-var-common.hh \
        $$PWD/src/hb-ot-var-avar-table.hh \
        $$PWD/src/hb-ot-var-cvar-table.hh \
        $$PWD/src/hb-ot-var-fvar-table.hh \
        $$PWD/src/hb-ot-var-gvar-table.hh \
        $$PWD/src/hb-ot-var-hvar-table.hh \
        $$PWD/src/hb-ot-var-mvar-table.hh

    HEADERS += \
        $$PWD/src/hb-ot.h \
        $$PWD/src/hb-ot-font.h \
        $$PWD/src/hb-ot-layout.h \
        $$PWD/src/hb-ot-math.h \
        $$PWD/src/hb-ot-shape.h \
        $$PWD/src/hb-ot-shape.hh \
        $$PWD/src/hb-ot-var.h \
        $$PWD/src/OT/Color/CBDT/CBDT.hh \
        $$PWD/src/OT/Color/COLR/COLR.hh \
        $$PWD/src/OT/Color/COLR/colrv1-closure.hh \
        $$PWD/src/OT/Color/CPAL/CPAL.hh \
        $$PWD/src/OT/Color/sbix/sbix.hh \
        $$PWD/src/OT/Color/svg/svg.hh \
        $$PWD/src/OT/Layout/GDEF/GDEF.hh \
        $$PWD/src/OT/name/name.hh

}

MODULE_EXT_HEADERS = $$HEADERS

contains(SHAPERS, coretext) {
    DEFINES += HAVE_CORETEXT

    SOURCES += \
        $$PWD/src/hb-coretext.cc

    HEADERS += \
        $$PWD/src/hb-coretext.h

    uikit: \
        # On iOS/tvOS/watchOS CoreText and CoreGraphics are stand-alone frameworks
        LIBS_PRIVATE += -framework CoreText -framework CoreGraphics
    else: \
        # On Mac OS they are part of the ApplicationServices umbrella framework,
        # even in 10.8 where they were also made available stand-alone.
        LIBS_PRIVATE += -framework ApplicationServices

    CONFIG += watchos_coretext
}

contains(SHAPERS, fallback)|isEmpty(SHAPERS) {
    DEFINES += HAVE_FALLBACK

    SOURCES += \
        $$PWD/src/hb-fallback-shape.cc
}

load(qt_helper_lib)
