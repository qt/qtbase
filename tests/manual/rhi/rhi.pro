TEMPLATE = subdirs

SUBDIRS += \
    hellominimalcrossgfxtriangle \
    compressedtexture_bc1 \
    compressedtexture_bc1_subupload \
    texuploads \
    msaatexture \
    msaarenderbuffer \
    cubemap \
    multiwindow \
    multiwindow_threaded \
    triquadcube \
    offscreen \
    floattexture \
    mrt \
    shadowmap

qtConfig(widgets) {
    SUBDIRS += \
        qrhiprof
}
