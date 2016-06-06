SOURCES = eglfs-egldevice.cpp

for(p, QMAKE_LIBDIR_EGL) {
    LIBS += -L$$p
}

INCLUDEPATH += $$QMAKE_INCDIR_EGL
LIBS += $$QMAKE_LIBS_EGL
CONFIG += link_pkgconfig
!contains(QT_CONFIG, no-pkg-config) {
    PKGCONFIG += libdrm
} else {
    LIBS += -ldrm
}
CONFIG -= qt
