# Generated from vkconvenience.pro.

qt_find_package(Vulkan) # special case

#####################################################################
## VulkanSupport Module:
#####################################################################

add_qt_module(VulkanSupport
    STATIC
    INTERNAL_MODULE
    SOURCES
        qbasicvulkanplatforminstance.cpp qbasicvulkanplatforminstance_p.h
        qvkconvenience.cpp qvkconvenience_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
    PRECOMPILED_HEADER
        "../../corelib/global/qt_pch.h"
)

#### Keys ignored in scope 1:.:.:vkconvenience.pro:<TRUE>:
# CONFIG = "static" "internal_module"
# MODULE = "vulkan_support"
# _LOADED = "qt_module"
