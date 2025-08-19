# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause
#####################################################################
## BundledHarfbuzz Generic Library:
#####################################################################

qt_internal_add_3rdparty_library(BundledHarfbuzz
    QMAKE_LIB_NAME harfbuzz
    STATIC
    SKIP_AUTOMOC
    SOURCES
        hb-dummy.cc
        src/hb.h
        src/hb-aat-layout.cc
        src/hb-aat-map.cc
        src/hb-algs.hh
        src/hb-atomic.hh
        src/hb-bit-vector.hh
        src/hb-blob.cc src/hb-blob.h
        src/hb-buffer.cc src/hb-buffer.h src/hb-buffer.hh
        src/hb-buffer-deserialize-json.hh
        src/hb-buffer-deserialize-text-glyphs.hh
        src/hb-buffer-deserialize-text-unicode.hh
        src/hb-buffer-serialize.cc
        src/hb-buffer-verify.cc
        src/hb-cache.hh
        src/hb-common.h
        src/hb-debug.hh
        src/hb-decycler.hh
        src/hb-deprecated.h
        src/hb-draw.cc src/hb-draw.h src/hb-draw.hh
        src/hb-face.cc src/hb-face.h src/hb-face.hh
        src/hb-face-builder.cc
        src/hb-fallback-shape.cc
        src/hb-font.cc src/hb-font.h src/hb-font.hh
        src/hb-ft-colr.hh
        src/hb-geometry.hh
        src/hb-limits.hh
        src/hb-map.cc
        src/hb-mutex.hh
        src/hb-number.cc
        src/hb-object.hh
        src/hb-open-file.hh
        src/hb-open-type.hh
        src/hb-outline.cc src/hb-outline.hh
        src/hb-paint.cc src/hb-paint.h src/hb-paint.hh
        src/hb-paint-bounded.cc src/hb-paint-bounded.hh
        src/hb-paint-extents.cc src/hb-paint-extents.hh
        src/hb-priority-queue.hh
        src/hb-repacker.hh
        src/hb-script-list.h
        src/hb-set.cc src/hb-set.h src/hb-set.hh
        src/hb-set-digest.hh
        src/hb-shape.cc src/hb-shape.h
        src/hb-shape-plan.cc src/hb-shape-plan.h src/hb-shape-plan.hh
        src/hb-shaper.cc src/hb-shaper.hh
        src/hb-shaper-impl.hh
        src/hb-shaper-list.hh
        src/hb-string-array.hh
        src/hb-style.cc src/hb-style.h
        src/hb-subset.cc
        src/hb-subset-cff-common.cc
        src/hb-subset-cff1.cc
        src/hb-subset-cff2.cc
        src/hb-subset-input.cc
        src/hb-subset-instancer-iup.cc src/hb-subset-instancer-iup.hh
        src/hb-subset-instancer-solver.cc
        src/hb-subset-plan.cc
        src/hb-subset-plan-layout.cc
        src/hb-subset-plan-member-list.hh
        src/hb-subset-serialize.cc src/hb-subset-serialize.h
        src/hb-subset-table-cff.cc
        src/hb-subset-table-color.cc
        src/hb-subset-table-layout.cc
        src/hb-subset-table-other.cc
        src/hb-subset-table-var.cc src/hb-subset-table.hh
        src/hb-subset-plan-var.cc
        src/hb-unicode.cc src/hb-unicode.h src/hb-unicode.hh
        src/hb-utf.hh
        src/hb-version.h
    DEFINES
        HAVE_ATEXIT
        HAVE_CONFIG_H
        HB_EXTERN=
        HB_NDEBUG
        HB_NO_UNICODE_FUNCS
        QT_NO_VERSION_TAGGING
    INCLUDE_DIRECTORIES
        "${CMAKE_CURRENT_SOURCE_DIR}"
        $<TARGET_PROPERTY:Core,INCLUDE_DIRECTORIES>
    PUBLIC_INCLUDE_DIRECTORIES
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/harfbuzz>
)

qt_internal_add_sync_header_dependencies(BundledHarfbuzz Core)

# GHS compiler doesn't support the __restrict keyword
if(INTEGRITY)
    target_compile_definitions(BundledHarfbuzz PRIVATE __restrict=)
endif()

qt_disable_warnings(BundledHarfbuzz)
qt_set_symbol_visibility_hidden(BundledHarfbuzz)

## Scopes:
#####################################################################

qt_internal_extend_target(BundledHarfbuzz CONDITION UNIX
    DEFINES
        HAVE_PTHREAD
        HAVE_SCHED_H
        HAVE_SCHED_YIELD
)

qt_internal_extend_target(BundledHarfbuzz CONDITION WIN32
    DEFINES
        HB_NO_WIN1256
)

qt_internal_extend_target(BundledHarfbuzz CONDITION TRUE
    SOURCES
        src/hb-ot.h
        src/hb-ot-cff1-std-str.hh
        src/hb-ot-cff1-table.cc
        src/hb-ot-cff2-table.cc
        src/hb-ot-cmap-table.hh
        src/hb-ot-color.cc
        src/hb-ot-face.cc
        src/hb-ot-font.cc src/hb-ot-font.h
        src/hb-ot-glyf-table.hh
        src/hb-ot-head-table.hh
        src/hb-ot-hhea-table.hh
        src/hb-ot-hmtx-table.hh
        src/hb-ot-kern-table.hh
        src/hb-ot-layout.cc src/hb-ot-layout.h src/hb-ot-layout.hh
        src/hb-ot-layout-gdef-table.hh
        src/hb-ot-layout-gpos-table.hh
        src/hb-ot-layout-gsub-table.hh
        src/hb-ot-layout-jstf-table.hh
        src/hb-ot-map.cc src/hb-ot-map.hh
        src/hb-ot-math.cc src/hb-ot-math.h
        src/hb-ot-math-table.hh
        src/hb-ot-maxp-table.hh
        src/hb-ot-meta.cc
        src/hb-ot-metrics.cc
        src/hb-ot-name.cc
        src/hb-ot-name-table.hh
        src/hb-ot-os2-table.hh
        src/hb-ot-post-macroman.hh
        src/hb-ot-post-table.hh
        src/hb-ot-post-table-v2subset.hh
        src/hb-ot-shape.cc src/hb-ot-shape.h src/hb-ot-shape.hh
        src/hb-ot-shape-fallback.cc src/hb-ot-shape-fallback.hh
        src/hb-ot-shape-normalize.cc src/hb-ot-shape-normalize.hh
        src/hb-ot-shaper-arabic-fallback.hh
        src/hb-ot-shaper-arabic-joining-list.hh
        src/hb-ot-shaper-arabic-pua.hh
        src/hb-ot-shaper-arabic-table.hh
        src/hb-ot-shaper-arabic.cc src/hb-ot-shaper-arabic.hh
        src/hb-ot-shaper-default.cc
        src/hb-ot-shaper-hangul.cc
        src/hb-ot-shaper-hebrew.cc
        src/hb-ot-shaper-indic-machine.hh
        src/hb-ot-shaper-indic-table.cc
        src/hb-ot-shaper-indic.cc src/hb-ot-shaper-indic.hh
        src/hb-ot-shaper-khmer-machine.hh
        src/hb-ot-shaper-khmer.cc
        src/hb-ot-shaper-myanmar-machine.hh
        src/hb-ot-shaper-myanmar.cc
        src/hb-ot-shaper-syllabic.cc src/hb-ot-shaper-syllabic.hh
        src/hb-ot-shaper-thai.cc
        src/hb-ot-shaper-use-machine.hh
        src/hb-ot-shaper-use-table.hh
        src/hb-ot-shaper-use.cc
        src/hb-ot-shaper-vowel-constraints.cc
        src/hb-ot-shaper-vowel-constraints.hh
        src/hb-ot-shaper.hh
        src/hb-subset-accelerator.hh
        src/hb-ot-tag.cc
        src/hb-ot-var.cc src/hb-ot-var.h
        src/hb-ot-var-common.hh
        src/hb-ot-var-avar-table.hh
        src/hb-ot-var-fvar-table.hh
        src/hb-ot-var-hvar-table.hh
        src/hb-ot-var-mvar-table.hh
        src/hb-ot-var-varc-table.hh
        src/OT/Color/CBDT/CBDT.hh
        src/OT/Color/COLR/COLR.hh
        src/OT/Color/COLR/colrv1-closure.hh
        src/OT/Color/CPAL/CPAL.hh
        src/OT/Color/sbix/sbix.hh
        src/OT/Color/svg/svg.hh
        src/OT/Layout/GDEF/GDEF.hh
        src/OT/name/name.hh
        src/OT/Var/VARC/coord-setter.hh
        src/OT/Var/VARC/VARC.cc src/OT/Var/VARC/VARC.hh
    DEFINES
        HAVE_OT
)

qt_internal_extend_target(BundledHarfbuzz CONDITION SHAPERS_ISEMPTY OR SHAPERS___contains___fallback
    SOURCES
        src/hb-fallback-shape.cc
    DEFINES
        HAVE_FALLBACK
)

qt_internal_add_3rdparty_header_module(HarfbuzzPrivate
    EXTERNAL_HEADERS_DIR include
)
