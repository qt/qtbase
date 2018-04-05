TEMPLATE = subdirs
SUBDIRS = \
        drawtexture \
        qcolor \
        qpainter \
        qregion \
        qtransform \
        qtbench \
        lancebench

!qtHaveModule(widgets): SUBDIRS -= \
    qpainter \
    qtbench
