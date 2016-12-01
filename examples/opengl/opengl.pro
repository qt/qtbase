TEMPLATE      = subdirs

SUBDIRS = paintedwindow \
          qopenglwindow
!emscripten: SUBDIRS +=  \
          hellowindow

qtHaveModule(widgets) {
    SUBDIRS += contextinfo \
               2dpainting \
               hellogl2
!emscripten: SUBDIRS +=  \
               threadedqopenglwidget \

    !wince: SUBDIRS += \
                qopenglwidget \
                cube \
                textures \
                hellogles3 \
                computegles31
}

EXAMPLE_FILES += \
    legacy
