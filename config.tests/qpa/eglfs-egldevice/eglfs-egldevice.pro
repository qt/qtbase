SOURCES = eglfs-egldevice.cpp

for(p, QMAKE_LIBDIR_EGL) {
    exists($$p):LIBS += -L$$p
}

INCLUDEPATH += $$QMAKE_INCDIR_EGL
LIBS += $$QMAKE_LIBS_EGL

LIBS += -ldrm

CONFIG -= qt
