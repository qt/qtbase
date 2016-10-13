qtConfig(system-freetype) {
    QMAKE_USE_PRIVATE += freetype/nolink
} else: qtConfig(freetype) {
    INCLUDEPATH += $$PWD/freetype/include
    LIBS_PRIVATE += -L$$QT_BUILD_TREE/lib -lqtfreetype$$qtPlatformTargetSuffix()
}
