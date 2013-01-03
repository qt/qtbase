TEMPLATE = subdirs
SUBDIRS = \
        qpainter \
        qregion \
        qtransform \
        qtracebench \
        qtbench

!qtHaveModule(widgets): SUBDIRS -= \
    qpainter \
    qtracebench \
    qtbench
