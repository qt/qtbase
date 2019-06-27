TARGET = QtVulkanSupport
MODULE = vulkan_support

QT = core-private gui-private
CONFIG += static internal_module

DEFINES += QT_NO_CAST_FROM_ASCII
PRECOMPILED_HEADER = ../../corelib/global/qt_pch.h

SOURCES += \
    qvkconvenience.cpp \
    qbasicvulkanplatforminstance.cpp

HEADERS += \
    qvkconvenience_p.h \
    qbasicvulkanplatforminstance_p.h

load(qt_module)
