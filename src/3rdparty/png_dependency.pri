contains(QT_CONFIG, system-png) {
    QMAKE_USE_PRIVATE += libpng
} else: contains(QT_CONFIG, png) {
    INCLUDEPATH += $$PWD/libpng
    LIBS_PRIVATE += -L$$QT_BUILD_TREE/lib -lqtpng$$qtPlatformTargetSuffix()
}
