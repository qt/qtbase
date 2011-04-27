TEMPLATE = lib
TARGET = pvrQWSWSEGL
CONFIG += dll warn_on
CONFIG -= qt

HEADERS+=\
    pvrqwsdrawable.h \
    pvrqwsdrawable_p.h

SOURCES+=\
    pvrqwsdrawable.c \
    pvrqwswsegl.c

INCLUDEPATH += $$QMAKE_INCDIR_EGL

for(p, QMAKE_LIBDIR_EGL) {
    exists($$p):LIBS += -L$$p
}

LIBS += -lpvr2d

DESTDIR = $$QMAKE_LIBDIR_QT
target.path = $$[QT_INSTALL_LIBS]
INSTALLS += target

include(../powervr.pri)