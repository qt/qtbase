SOURCES = libpng.cpp
CONFIG -= qt dylib
!contains(QT_CONFIG, no-pkg-config) {
    CONFIG += link_pkgconfig
    PKGCONFIG += libpng
} else {
    LIBS += -lpng
}
