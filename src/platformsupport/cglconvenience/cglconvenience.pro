TARGET = QtCglSupport
MODULE = cgl_support

QT = core-private gui
CONFIG += static internal_module

DEFINES += QT_NO_CAST_FROM_ASCII
PRECOMPILED_HEADER = ../../corelib/global/qt_pch.h

HEADERS += \
    cglconvenience_p.h

OBJECTIVE_SOURCES += \
    cglconvenience.mm

LIBS_PRIVATE += -framework AppKit -framework OpenGL

load(qt_module)
