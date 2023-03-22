TEMPLATE      = subdirs

SUBDIRS = paintedwindow \
          openglwindow

qtHaveModule(widgets) {
    SUBDIRS += contextinfo \
               2dpainting \
               hellogl2 \
               qopenglwidget \
               cube \
               textures \
               hellogles3 \
               stereoqopenglwidget
}
