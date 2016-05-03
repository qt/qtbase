SOURCES = eglfs-brcm.cpp

CONFIG -= qt

INCLUDEPATH += $$QMAKE_INCDIR_EGL

for(p, QMAKE_LIBDIR_EGL) {
    LIBS += -L$$p
}

LIBS += -lEGL -lGLESv2 -lbcm_host
