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

!contains(QT_CONFIG, opengl(es2)?): SUBDIRS -= qopengl
