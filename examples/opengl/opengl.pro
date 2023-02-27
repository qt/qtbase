TEMPLATE      = subdirs

SUBDIRS = hellowindow \
          paintedwindow \
          openglwindow \
          qopenglwindow

qtHaveModule(widgets) {
    SUBDIRS += contextinfo \
               2dpainting \
               hellogl2 \
               qopenglwidget \
               cube \
               textures \
               hellogles3 \
               computegles31 \
               stereoqopenglwidget
}
