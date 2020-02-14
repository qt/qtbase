# Generated from harfbuzz-ng.pro.

#####################################################################
## BundledHarfbuzz Generic Library:
#####################################################################

qt_add_3rdparty_library(BundledHarfbuzz
    STATIC
    SOURCES
        src/hb.h
        src/hb-atomic-private.hh
        src/hb-blob.cc src/hb-blob.h
        src/hb-buffer.cc src/hb-buffer.h
        src/hb-buffer-deserialize-json.hh
        src/hb-buffer-deserialize-text.hh
        src/hb-buffer-private.hh
        src/hb-buffer-serialize.cc
        src/hb-cache-private.hh
        src/hb-common.cc src/hb-common.h
        src/hb-debug.hh
        src/hb-deprecated.h
        src/hb-dsalgs.hh
        src/hb-face.cc src/hb-face.h
        src/hb-face-private.hh
        src/hb-font.cc src/hb-font.h
        src/hb-font-private.hh
        src/hb-mutex-private.hh
        src/hb-object-private.hh
        src/hb-open-file-private.hh
        src/hb-open-type-private.hh
        src/hb-ot-cbdt-table.hh
        src/hb-ot-cmap-table.hh
        src/hb-ot-glyf-table.hh
        src/hb-ot-head-table.hh
        src/hb-ot-hhea-table.hh
        src/hb-ot-hmtx-table.hh
        src/hb-ot-maxp-table.hh
        src/hb-ot-name-table.hh
        src/hb-ot-os2-table.hh
        src/hb-ot-post-table.hh
        src/hb-ot-tag.cc
        src/hb-private.hh
        src/hb-set.cc src/hb-set.h
        src/hb-set-digest-private.hh
        src/hb-set-private.hh
        src/hb-shape.cc src/hb-shape.h
        src/hb-shape-plan.cc src/hb-shape-plan.h
        src/hb-shape-plan-private.hh
        src/hb-shaper.cc
        src/hb-shaper-impl-private.hh
        src/hb-shaper-list.hh
        src/hb-shaper-private.hh
        src/hb-string-array.hh
        src/hb-unicode.cc src/hb-unicode.h
        src/hb-unicode-private.hh
        src/hb-utf-private.hh
        src/hb-version.h
        src/hb-warning.cc
    DEFINES
        HAVE_ATEXIT
        HAVE_CONFIG_H
        HB_DISABLE_DEPRECATED
        HB_EXTERN=
        HB_NDEBUG
        HB_NO_UNICODE_FUNCS
        QT_NO_VERSION_TAGGING
    INCLUDE_DIRECTORIES
        $<TARGET_PROPERTY:Core,INCLUDE_DIRECTORIES> # special case
        "${CMAKE_CURRENT_SOURCE_DIR}" # special case
    PUBLIC_INCLUDE_DIRECTORIES
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
qt_disable_warnings(BundledHarfbuzz)
qt_set_symbol_visibility_hidden(BundledHarfbuzz)

#### Keys ignored in scope 1:.:.:harfbuzz-ng.pro:<TRUE>:
# SHAPERS = "opentype"

## Scopes:
#####################################################################

qt_extend_target(BundledHarfbuzz CONDITION APPLE
    SOURCES
        src/hb-coretext.cc src/hb-coretext.h
    DEFINES
        HAVE_CORETEXT
)

#### Keys ignored in scope 2:.:.:harfbuzz-ng.pro:APPLE:
# SHAPERS = "coretext"

qt_extend_target(BundledHarfbuzz CONDITION UNIX
    DEFINES
        HAVE_PTHREAD
        HAVE_SCHED_H
        HAVE_SCHED_YIELD
)

qt_extend_target(BundledHarfbuzz CONDITION WIN32
    DEFINES
        HB_NO_WIN1256
)

qt_extend_target(BundledHarfbuzz CONDITION ANDROID
    DEFINES
        _POSIX_C_SOURCE=200112L
)

qt_extend_target(BundledHarfbuzz CONDITION TRUE # special case
    SOURCES
        src/hb-ot.h
        src/hb-ot-font.cc src/hb-ot-font.h
        src/hb-ot-kern-table.hh
        src/hb-ot-layout.cc src/hb-ot-layout.h
        src/hb-ot-layout-common-private.hh
        src/hb-ot-layout-gdef-table.hh
        src/hb-ot-layout-gpos-table.hh
        src/hb-ot-layout-gsub-table.hh
        src/hb-ot-layout-gsubgpos-private.hh
        src/hb-ot-layout-jstf-table.hh
        src/hb-ot-layout-math-table.hh
        src/hb-ot-layout-private.hh
        src/hb-ot-map.cc
        src/hb-ot-map-private.hh
        src/hb-ot-math.cc src/hb-ot-math.h
        src/hb-ot-math-table.hh
        src/hb-ot-post-macroman.hh
        src/hb-ot-shape.cc src/hb-ot-shape.h
        src/hb-ot-shape-complex-arabic.cc
        src/hb-ot-shape-complex-arabic-fallback.hh
        src/hb-ot-shape-complex-arabic-private.hh
        src/hb-ot-shape-complex-arabic-table.hh
        src/hb-ot-shape-complex-default.cc
        src/hb-ot-shape-complex-hangul.cc
        src/hb-ot-shape-complex-hebrew.cc
        src/hb-ot-shape-complex-indic.cc
        src/hb-ot-shape-complex-indic-machine.hh
        src/hb-ot-shape-complex-indic-private.hh
        src/hb-ot-shape-complex-indic-table.cc
        src/hb-ot-shape-complex-myanmar.cc
        src/hb-ot-shape-complex-myanmar-machine.hh
        src/hb-ot-shape-complex-private.hh
        src/hb-ot-shape-complex-thai.cc
        src/hb-ot-shape-complex-tibetan.cc
        src/hb-ot-shape-complex-use.cc
        src/hb-ot-shape-complex-use-machine.hh
        src/hb-ot-shape-complex-use-private.hh
        src/hb-ot-shape-complex-use-table.cc
        src/hb-ot-shape-fallback.cc
        src/hb-ot-shape-fallback-private.hh
        src/hb-ot-shape-normalize.cc
        src/hb-ot-shape-normalize-private.hh
        src/hb-ot-shape-private.hh
        src/hb-ot-tag.h
        src/hb-ot-var.cc src/hb-ot-var.h
        src/hb-ot-var-avar-table.hh
        src/hb-ot-var-fvar-table.hh
        src/hb-ot-var-hvar-table.hh
        src/hb-ot-var-mvar-table.hh
    DEFINES
        HAVE_OT
)

qt_extend_target(BundledHarfbuzz CONDITION APPLE_UIKIT
    LIBRARIES
        ${FWCoreGraphics}
        ${FWCoreText}
)

qt_extend_target(BundledHarfbuzz CONDITION APPLE AND NOT APPLE_UIKIT
    LIBRARIES
        ${FWApplicationServices}
)

qt_extend_target(BundledHarfbuzz CONDITION SHAPERS_ISEMPTY OR SHAPERS___contains___fallback
    SOURCES
        src/hb-fallback-shape.cc
    DEFINES
        HAVE_FALLBACK
)
