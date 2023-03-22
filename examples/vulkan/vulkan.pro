TEMPLATE = subdirs

SUBDIRS = hellovulkantriangle

qtHaveModule(widgets) {
    SUBDIRS += hellovulkanwidget
    qtHaveModule(concurrent): SUBDIRS += hellovulkancubes
}
