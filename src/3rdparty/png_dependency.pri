contains(QT_CONFIG, system-png) {
    unix|mingw: LIBS_PRIVATE += -lpng
    else: LIBS += libpng.lib
} else: contains(QT_CONFIG, png) {
    INCLUDEPATH += $$PWD/libpng
    LIBS_PRIVATE += -L$$QT_BUILD_TREE/lib -lqtpng$$qtPlatformTargetSuffix()
}
