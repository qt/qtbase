TEMPLATE = subdirs

SUBDIRS += \
    hellominimalcrossgfxtriangle \
    compressedtexture_bc1 \
    compressedtexture_bc1_subupload \
    texuploads \
    msaatexture \
    msaarenderbuffer \
    cubemap \
    cubemap_scissor \
    multiwindow \
    multiwindow_threaded \
    triquadcube \
    offscreen \
    floattexture \
    mrt \
    shadowmap \
    computebuffer \
    computeimage \
    instancing

qtConfig(widgets) {
    SUBDIRS += \
        qrhiprof
}
