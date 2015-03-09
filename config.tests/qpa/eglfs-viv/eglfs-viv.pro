SOURCES = eglfs-viv.cpp
DEFINES += LINUX=1 EGL_API_FB=1

CONFIG -= qt

LIBS += -lEGL -lGLESv2 -lGAL
