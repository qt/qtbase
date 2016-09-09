qtConfig(system-pcre) {
    QMAKE_USE_PRIVATE += pcre
} else {
    win32: DEFINES += PCRE_STATIC
    INCLUDEPATH += $$PWD/pcre
    LIBS_PRIVATE += -L$$QT_BUILD_TREE/lib -lqtpcre$$qtPlatformTargetSuffix()
}
