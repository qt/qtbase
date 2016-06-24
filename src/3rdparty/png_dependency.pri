contains(QT_CONFIG, system-png) {
    unix|mingw {
        !contains(QT_CONFIG, no-pkg-config) {
            CONFIG += link_pkgconfig
            PKGCONFIG_PRIVATE += libpng
        } else {
            LIBS_PRIVATE += -lpng
        }
    } else {
        LIBS += libpng.lib
    }
} else: contains(QT_CONFIG, png) {
    include($$PWD/libpng.pri)
}
