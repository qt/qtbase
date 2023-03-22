TEMPLATE      = subdirs

SUBDIRS = openglwindow \
          hellogles3

qtHaveModule(widgets) {
    SUBDIRS += contextinfo \
               2dpainting \
               hellogl2 \
               qopenglwidget \
               cube \
               textures \
               stereoqopenglwidget
}
