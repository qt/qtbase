TARGET = QtPlatformCompositorSupport
MODULE = platformcompositor_support

QT = core-private gui-private
CONFIG += static internal_module

DEFINES += QT_NO_CAST_FROM_ASCII

SOURCES += \
    qplatformbackingstoreopenglsupport.cpp \
    qopenglcompositor.cpp \
    qopenglcompositorbackingstore.cpp

HEADERS += \
    qplatformbackingstoreopenglsupport.h \
    qopenglcompositor_p.h \
    qopenglcompositorbackingstore_p.h

load(qt_module)
