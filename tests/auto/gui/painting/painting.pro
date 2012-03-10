TEMPLATE=subdirs
SUBDIRS=\
   qpainterpath \
   qpainterpathstroker \
   qcolor \
   qbrush \
   qregion \
   qpainter \
   qpathclipper \
   qpen \
   qpaintengine \
   qtransform \
   qwmatrix \
   qpolygon \

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
    qpathclipper \


