# Generated from vulkan.pro.

qt_internal_add_example(hellovulkanwindow)
qt_internal_add_example(hellovulkantriangle)
qt_internal_add_example(hellovulkantexture)
if(TARGET Qt::Widgets)
    qt_internal_add_example(hellovulkanwidget)
endif()
if(TARGET Qt::Concurrent AND TARGET Qt::Widgets)
    qt_internal_add_example(hellovulkancubes)
endif()
