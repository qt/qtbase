find_package(Vulkan) # special case

#####################################################################
## VulkanSupport Module:
#####################################################################

add_qt_module(VulkanSupport
    STATIC
    SOURCES
        qbasicvulkanplatforminstance.cpp qbasicvulkanplatforminstance_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
)

#### Keys ignored in scope 1:.:vkconvenience.pro:<NONE>:
# CONFIG = "static" "internal_module"
# MODULE = "vulkan_support"
# PRECOMPILED_HEADER = "../../corelib/global/qt_pch.h"
# _LOADED = "qt_module"
