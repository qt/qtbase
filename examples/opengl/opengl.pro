TEMPLATE      = subdirs

SUBDIRS = hellowindow \
          paintedwindow \
          qopenglwindow

qtHaveModule(widgets) {
    SUBDIRS += contextinfo \
               threadedqopenglwidget \
               2dpainting \
               hellogl2

    !wince: SUBDIRS += \
                qopenglwidget \
                cube \
                textures \
                hellogles3
}
