TEMPLATE = subdirs

SUBDIRS = hellovulkanwindow \
          hellovulkantriangle \
          hellovulkantexture

qtHaveModule(widgets): SUBDIRS += hellovulkanwidget
