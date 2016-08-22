qtConfig(system-png) {
    QMAKE_USE_PRIVATE += libpng
} else: qtConfig(png) {
    INCLUDEPATH += $$PWD/libpng
    LIBS_PRIVATE += -L$$QT_BUILD_TREE/lib -lqtpng$$qtPlatformTargetSuffix()
}
