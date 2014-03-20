requires(qtHaveModule(opengl))

TEMPLATE      = subdirs

contains(QT_CONFIG, dynamicgl) {
    SUBDIRS = hellowindow \
              contextinfo \
              qopenglwidget \
              threadedqopenglwidget \
              qopenglwindow
} else: !contains(QT_CONFIG, opengles2) {
    SUBDIRS   = 2dpainting \
                grabber \
                hellogl \
                overpainting \
                pbuffers \
                framebufferobject2 \
                samplebuffers
}

!contains(QT_CONFIG, dynamicgl): SUBDIRS += hellowindow \
           paintedwindow \
           contextinfo \
           cube \
           textures \
           qopenglwidget \
           threadedqopenglwidget \
           qopenglwindow

EXAMPLE_FILES = shared
