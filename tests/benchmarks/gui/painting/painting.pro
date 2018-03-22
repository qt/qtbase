TEMPLATE = subdirs
SUBDIRS = \
        drawtexture \
        qcolor \
        qpainter \
        qregion \
        qtransform \
        qtbench

!qtHaveModule(widgets): SUBDIRS -= \
    qpainter \
    qtbench
