TEMPLATE      = subdirs

SUBDIRS = openglwindow \
          hellogles3

qtHaveModule(widgets) {
    SUBDIRS += 2dpainting \
               hellogl2 \
               cube \
               textures \
               stereoqopenglwidget
}
