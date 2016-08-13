SOURCES = kms.cpp
!contains(QT_CONFIG, no-pkg-config) {
    CONFIG += link_pkgconfig
    PKGCONFIG += libdrm
} else {
    LIBS += -ldrm
}
CONFIG -= qt
