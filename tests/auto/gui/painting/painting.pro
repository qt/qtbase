TEMPLATE=subdirs
SUBDIRS=\
   qpainterpath \
   qpainterpathstroker \
   qcolor \
   qbrush \
   qregion \
   qpagesize \
   qpainter \
   qpathclipper \
   qpen \
   qpaintengine \
   qtransform \
   qwmatrix \
   qpolygon \

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
    qpathclipper \


