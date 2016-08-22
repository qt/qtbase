SOURCES = eglfs-viv.cpp
integrity {
  DEFINES += EGL_API_FB=1
} else {
  DEFINES += LINUX=1 EGL_API_FB=1
}
CONFIG -= qt
