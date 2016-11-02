TARGET = QtPlatformCompositorSupport
MODULE = platformcompositor_support

QT = core-private gui-private
CONFIG += static internal_module

DEFINES += QT_NO_CAST_FROM_ASCII
PRECOMPILED_HEADER = ../../corelib/global/qt_pch.h

SOURCES += \
    qopenglcompositor.cpp \
    qopenglcompositorbackingstore.cpp

HEADERS += \
    qopenglcompositor_p.h \
    qopenglcompositorbackingstore_p.h

load(qt_module)
