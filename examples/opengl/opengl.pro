requires(qtHaveModule(opengl))

TEMPLATE      = subdirs

contains(QT_CONFIG, opengles2) {
    SUBDIRS   = hellogl_es2
} else {
    SUBDIRS   = 2dpainting \
                grabber \
                hellogl \
                overpainting \
                pbuffers \
                framebufferobject2 \
                samplebuffers
}

SUBDIRS += hellowindow \
           paintedwindow \
           contextinfo \
           cube \
           textures

EXAMPLE_FILES = shared
