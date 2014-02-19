TEMPLATE=subdirs

SUBDIRS = \
    kernel

!ios: SUBDIRS += \
    image \
    math3d \
    painting \
    qopengl \
    text \
    util \
    itemmodels \

!contains(QT_CONFIG, opengl(es1|es2)?): SUBDIRS -= qopengl
