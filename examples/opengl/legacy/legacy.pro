requires(qtHaveModule(opengl))

TEMPLATE      = subdirs

!qtConfig(dynamicgl):!qtConfig(opengles2) {
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
