qtConfig(system-freetype) {
    QMAKE_USE_PRIVATE += freetype/nolink
} else: qtConfig(freetype) {
    QMAKE_USE_PRIVATE += freetype
}
