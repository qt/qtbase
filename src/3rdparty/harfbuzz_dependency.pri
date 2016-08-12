contains(QT_CONFIG, system-harfbuzz) {
    QMAKE_USE_PRIVATE += harfbuzz
} else: contains(QT_CONFIG, harfbuzz) {
    INCLUDEPATH += $$PWD/harfbuzz-ng/include
    LIBS_PRIVATE += -L$$QT_BUILD_TREE/lib -lqtharfbuzzng$$qtPlatformTargetSuffix()
}
