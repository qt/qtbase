TARGET = QtKmsSupport
MODULE = kms_support

QT = core-private gui-private
CONFIG += static internal_module

DEFINES += QT_NO_CAST_FROM_ASCII

HEADERS += \
    qkmsdevice_p.h

SOURCES += \
    qkmsdevice.cpp

QMAKE_USE += drm

LIBS_PRIVATE += $$QMAKE_LIBS_DYNLOAD

load(qt_module)
