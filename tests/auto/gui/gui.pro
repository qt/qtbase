TEMPLATE=subdirs

SUBDIRS = \
    kernel

!uikit: SUBDIRS += \
    image \
    math3d \
    painting \
    qopenglconfig \
    qopengl \
    text \
    util \
    itemmodels \

!qtConfig(opengl(es2)?): SUBDIRS -= qopengl qopenglconfig
