requires(qtHaveModule(opengl))

TEMPLATE      = subdirs

!contains(QT_CONFIG, dynamicgl): !contains(QT_CONFIG, opengles2) {
    # legacy desktop-only examples, no dynamic GL support
    SUBDIRS   = \
                grabber \
                hellogl \
                overpainting \
                pbuffers \
                framebufferobject2 \
                samplebuffers
}

EXAMPLE_FILES = shared
