TEMPLATE      = subdirs

SUBDIRS = openglwindow

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
