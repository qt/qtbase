TEMPLATE=subdirs
SUBDIRS=\
   qpainterpath \
   qpainterpathstroker \
   qcolor \
   qbrush \
   qregion \
   qpainter \
   qpathclipper \
   qprinterinfo \
   qpen \
   qpaintengine \
   qtransform \
   qwmatrix \
   qprinter \
   qpolygon \

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
    qpathclipper \


