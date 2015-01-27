TEMPLATE = subdirs
SUBDIRS = \
        qpainter \
        qregion \
        qtransform \
        qtbench

!qtHaveModule(widgets): SUBDIRS -= \
    qpainter \
    qtracebench \
    qtbench
