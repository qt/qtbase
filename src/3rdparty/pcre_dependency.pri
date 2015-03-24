pcre {
    win32: DEFINES += PCRE_STATIC
    INCLUDEPATH += $$PWD/pcre
    LIBS_PRIVATE += -L$$QT_BUILD_TREE/lib -lqtpcre$$qtPlatformTargetSuffix()
} else {
    LIBS_PRIVATE += -lpcre16
}
