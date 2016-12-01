TEMPLATE      = subdirs
SUBDIRS       = basicdrawing \
                concentriccircles \
                affine \
                composition \
                gradients \
                pathstroke \
                imagecomposition \
                painterpaths \
                transformations \
                fontsampler

EXAMPLE_FILES = \
    shared
!emscripten: SUBDIRS += \
                deform
