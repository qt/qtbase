SOURCES = eglfs-brcm.cpp

CONFIG -= qt

INCLUDEPATH += $$QMAKE_INCDIR_EGL

for(p, QMAKE_LIBDIR_EGL) {
    exists($$p):LIBS += -L$$p
}

LIBS += -lEGL -lGLESv2 -lbcm_host
