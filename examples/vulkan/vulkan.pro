TEMPLATE = subdirs

SUBDIRS = hellovulkanwindow \
          hellovulkantriangle

qtHaveModule(widgets) {
    SUBDIRS += hellovulkanwidget
    qtHaveModule(concurrent): SUBDIRS += hellovulkancubes
}
