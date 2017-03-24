TEMPLATE = subdirs

SUBDIRS = hellovulkanwindow \
          hellovulkantriangle \
          hellovulkantexture

qtHaveModule(widgets) {
    SUBDIRS += hellovulkanwidget
    qtHaveModule(concurrent): SUBDIRS += hellovulkancubes
}
