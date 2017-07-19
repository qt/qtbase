TEMPLATE      = subdirs

SUBDIRS = hellowindow \
          paintedwindow \
          qopenglwindow

qtHaveModule(widgets) {
    SUBDIRS += contextinfo \
               threadedqopenglwidget \
               2dpainting \
               hellogl2 \
               qopenglwidget \
               cube \
               textures \
               hellogles3 \
               computegles31
}

EXAMPLE_FILES += \
    legacy
