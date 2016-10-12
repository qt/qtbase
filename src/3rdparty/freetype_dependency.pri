qtConfig(system-freetype) {
    QMAKE_USE += freetype/nolink
} else: qtConfig(freetype) {
    QMAKE_USE += freetype
}
