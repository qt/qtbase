SOURCES = eglfs-brcm.cpp

CONFIG -= qt

INCLUDEPATH += $$QMAKE_INCDIR_EGL

LIBS += -L$$QMAKE_LIBDIR_EGL -lEGL -lGLESv2 -lbcm_host
