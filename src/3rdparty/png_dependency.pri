contains(QT_CONFIG, system-png) {
    unix|mingw: LIBS_PRIVATE += -lpng
    else: LIBS += libpng.lib
} else: contains(QT_CONFIG, png) {
    include($$PWD/libpng.pri)
}
