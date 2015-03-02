SOURCES = eglfs-brcm.cpp

CONFIG -= qt

INCLUDEPATH += $$[QT_SYSROOT]/opt/vc/include \
               $$[QT_SYSROOT]/opt/vc/include/interface/vcos/pthreads \
               $$[QT_SYSROOT]/opt/vc/include/interface/vmcs_host/linux

LIBS += -L$$[QT_SYSROOT]/opt/vc/lib -lEGL -lGLESv2 -lbcm_host
