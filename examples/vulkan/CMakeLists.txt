# Generated from vulkan.pro.

add_subdirectory(hellovulkanwindow)
add_subdirectory(hellovulkantriangle)
add_subdirectory(hellovulkantexture)
if(TARGET Qt::Widgets)
    add_subdirectory(hellovulkanwidget)
endif()
if(TARGET Qt::Concurrent AND TARGET Qt::Widgets)
    add_subdirectory(hellovulkancubes)
endif()
