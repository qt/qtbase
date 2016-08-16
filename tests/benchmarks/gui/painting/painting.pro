TEMPLATE = subdirs
SUBDIRS = \
        qcolor \
        qpainter \
        qregion \
        qtransform \
        qtbench

!qtHaveModule(widgets): SUBDIRS -= \
    qpainter \
    qtracebench \
    qtbench
