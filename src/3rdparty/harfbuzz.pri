contains(QT_CONFIG, harfbuzz) {
    QT_HARFBUZZ_DIR = $$QT_SOURCE_TREE/src/3rdparty/harfbuzz-ng

    INCLUDEPATH += $$QT_HARFBUZZ_DIR/include

    SOURCES += \
        $$QT_HARFBUZZ_DIR/src/hb-blob.cc \
        $$QT_HARFBUZZ_DIR/src/hb-buffer.cc \
        $$QT_HARFBUZZ_DIR/src/hb-buffer-serialize.cc \
        $$QT_HARFBUZZ_DIR/src/hb-common.cc \
        $$QT_HARFBUZZ_DIR/src/hb-face.cc \
        $$QT_HARFBUZZ_DIR/src/hb-fallback-shape.cc \
        $$QT_HARFBUZZ_DIR/src/hb-font.cc \
        $$QT_HARFBUZZ_DIR/src/hb-ot-tag.cc \
        $$QT_HARFBUZZ_DIR/src/hb-set.cc \
        $$QT_HARFBUZZ_DIR/src/hb-shape.cc \
        $$QT_HARFBUZZ_DIR/src/hb-shape-plan.cc \
        $$QT_HARFBUZZ_DIR/src/hb-shaper.cc \
        $$QT_HARFBUZZ_DIR/src/hb-unicode.cc \
        $$QT_HARFBUZZ_DIR/src/hb-warning.cc

    HEADERS += \
        $$QT_HARFBUZZ_DIR/src/hb-atomic-private.hh \
        $$QT_HARFBUZZ_DIR/src/hb-buffer-private.hh \
        $$QT_HARFBUZZ_DIR/src/hb-buffer-deserialize-json.hh \
        $$QT_HARFBUZZ_DIR/src/hb-buffer-deserialize-text.hh \
        $$QT_HARFBUZZ_DIR/src/hb-cache-private.hh \
        $$QT_HARFBUZZ_DIR/src/hb-face-private.hh \
        $$QT_HARFBUZZ_DIR/src/hb-font-private.hh \
        $$QT_HARFBUZZ_DIR/src/hb-mutex-private.hh \
        $$QT_HARFBUZZ_DIR/src/hb-object-private.hh \
        $$QT_HARFBUZZ_DIR/src/hb-open-file-private.hh \
        $$QT_HARFBUZZ_DIR/src/hb-open-type-private.hh \
        $$QT_HARFBUZZ_DIR/src/hb-ot-head-table.hh \
        $$QT_HARFBUZZ_DIR/src/hb-ot-hhea-table.hh \
        $$QT_HARFBUZZ_DIR/src/hb-ot-hmtx-table.hh \
        $$QT_HARFBUZZ_DIR/src/hb-ot-maxp-table.hh \
        $$QT_HARFBUZZ_DIR/src/hb-ot-name-table.hh \
        $$QT_HARFBUZZ_DIR/src/hb-private.hh \
        $$QT_HARFBUZZ_DIR/src/hb-set-private.hh \
        $$QT_HARFBUZZ_DIR/src/hb-shape-plan-private.hh \
        $$QT_HARFBUZZ_DIR/src/hb-shaper-impl-private.hh \
        $$QT_HARFBUZZ_DIR/src/hb-shaper-list.hh \
        $$QT_HARFBUZZ_DIR/src/hb-shaper-private.hh \
        $$QT_HARFBUZZ_DIR/src/hb-unicode-private.hh \
        $$QT_HARFBUZZ_DIR/src/hb-utf-private.hh

    HEADERS += \
        $$QT_HARFBUZZ_DIR/src/hb.h \
        $$QT_HARFBUZZ_DIR/src/hb-blob.h \
        $$QT_HARFBUZZ_DIR/src/hb-buffer.h \
        $$QT_HARFBUZZ_DIR/src/hb-common.h \
        $$QT_HARFBUZZ_DIR/src/hb-face.h \
        $$QT_HARFBUZZ_DIR/src/hb-font.h \
        $$QT_HARFBUZZ_DIR/src/hb-set.h \
        $$QT_HARFBUZZ_DIR/src/hb-shape.h \
        $$QT_HARFBUZZ_DIR/src/hb-shape-plan.h \
        $$QT_HARFBUZZ_DIR/src/hb-unicode.h \
        $$QT_HARFBUZZ_DIR/src/hb-version.h

    # Open Type
    SOURCES += \
        $$QT_HARFBUZZ_DIR/src/hb-ot-layout.cc \
        $$QT_HARFBUZZ_DIR/src/hb-ot-map.cc \
        $$QT_HARFBUZZ_DIR/src/hb-ot-shape.cc \
        $$QT_HARFBUZZ_DIR/src/hb-ot-shape-complex-arabic.cc \
        $$QT_HARFBUZZ_DIR/src/hb-ot-shape-complex-default.cc \
        $$QT_HARFBUZZ_DIR/src/hb-ot-shape-complex-indic.cc \
        $$QT_HARFBUZZ_DIR/src/hb-ot-shape-complex-indic-table.cc \
        $$QT_HARFBUZZ_DIR/src/hb-ot-shape-complex-myanmar.cc \
        $$QT_HARFBUZZ_DIR/src/hb-ot-shape-complex-sea.cc \
        $$QT_HARFBUZZ_DIR/src/hb-ot-shape-complex-thai.cc \
        $$QT_HARFBUZZ_DIR/src/hb-ot-shape-fallback.cc \
        $$QT_HARFBUZZ_DIR/src/hb-ot-shape-normalize.cc

    HEADERS += \
        $$QT_HARFBUZZ_DIR/src/hb-ot-layout-common-private.hh \
        $$QT_HARFBUZZ_DIR/src/hb-ot-layout-gdef-table.hh \
        $$QT_HARFBUZZ_DIR/src/hb-ot-layout-gpos-table.hh \
        $$QT_HARFBUZZ_DIR/src/hb-ot-layout-gsubgpos-private.hh \
        $$QT_HARFBUZZ_DIR/src/hb-ot-layout-gsub-table.hh \
        $$QT_HARFBUZZ_DIR/src/hb-ot-layout-private.hh \
        $$QT_HARFBUZZ_DIR/src/hb-ot-map-private.hh \
        $$QT_HARFBUZZ_DIR/src/hb-ot-shape-complex-arabic-fallback.hh \
        $$QT_HARFBUZZ_DIR/src/hb-ot-shape-complex-arabic-table.hh \
        $$QT_HARFBUZZ_DIR/src/hb-ot-shape-complex-indic-machine.hh \
        $$QT_HARFBUZZ_DIR/src/hb-ot-shape-complex-indic-private.hh \
        $$QT_HARFBUZZ_DIR/src/hb-ot-shape-complex-myanmar-machine.hh \
        $$QT_HARFBUZZ_DIR/src/hb-ot-shape-complex-private.hh \
        $$QT_HARFBUZZ_DIR/src/hb-ot-shape-complex-sea-machine.hh \
        $$QT_HARFBUZZ_DIR/src/hb-ot-shape-fallback-private.hh \
        $$QT_HARFBUZZ_DIR/src/hb-ot-shape-normalize-private.hh \
        $$QT_HARFBUZZ_DIR/src/hb-ot-shape-private.hh

    HEADERS += \
        $$QT_HARFBUZZ_DIR/src/hb-ot.h \
        $$QT_HARFBUZZ_DIR/src/hb-ot-layout.h \
        $$QT_HARFBUZZ_DIR/src/hb-ot-tag.h

    DEFINES += HAVE_CONFIG_H
    QT += core-private

    TR_EXCLUDE += $$QT_HARFBUZZ_DIR/*
} else:contains(QT_CONFIG, system-harfbuzz) {
    LIBS_PRIVATE += -lharfbuzz
}
