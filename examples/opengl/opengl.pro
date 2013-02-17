requires(qtHaveModule(opengl))

TEMPLATE      = subdirs

contains(QT_CONFIG, opengles1)|contains(QT_CONFIG, opengles2){
    contains(QT_CONFIG, opengles2) {
        SUBDIRS   = hellogl_es2
    } else {
        SUBDIRS   = hellogl_es
    }
    SUBDIRS += textures
    contains(QT_CONFIG, opengles1) {
        SUBDIRS += hellogl
    }
} else {
    SUBDIRS   = 2dpainting \
                cube \
                grabber \
                hellogl \
                overpainting \
                pbuffers \
                framebufferobject2 \
                samplebuffers \
                textures
}

SUBDIRS += hellowindow \
           paintedwindow

EXAMPLE_FILES = shared
