TEMPLATE = subdirs
SUBDIRS = \
        qpainter \
        qregion \
        qtransform \
        qtracebench \
        qtbench

isEmpty(QT.widgets.name): SUBDIRS -= \
    qpainter \
    qtracebench \
    qtbench
