# Generated from vkconvenience.pro.

#####################################################################
## VulkanSupport Module:
#####################################################################

# Special case.
find_package(Vulkan)

add_qt_module(VulkanSupport
    STATIC
    SOURCES
        qbasicvulkanplatforminstance.cpp qbasicvulkanplatforminstance_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Vulkan::Vulkan
)
