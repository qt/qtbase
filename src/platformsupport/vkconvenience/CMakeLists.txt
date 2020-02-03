# Generated from vkconvenience.pro.

qt_find_package(Vulkan) # special case

#####################################################################
## VulkanSupport Module:
#####################################################################

qt_add_module(VulkanSupport
    STATIC
    INTERNAL_MODULE
    SOURCES
        qbasicvulkanplatforminstance.cpp qbasicvulkanplatforminstance_p.h
        qvkconvenience.cpp qvkconvenience_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PRECOMPILED_HEADER
        "../../corelib/global/qt_pch.h"
)

#### Keys ignored in scope 1:.:.:vkconvenience.pro:<TRUE>:
# MODULE = "vulkan_support"

## Scopes:
#####################################################################

qt_extend_target(VulkanSupport CONDITION QT_FEATURE_opengl
    PUBLIC_LIBRARIES
        Qt::OpenGL
)
