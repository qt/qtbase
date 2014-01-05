TEMPLATE=subdirs
SUBDIRS=\
   qpainterpath \
   qpainterpathstroker \
   qcolor \
   qbrush \
   qregion \
   qpagelayout \
   qpagesize \
   qpainter \
   qpathclipper \
   qpdfwriter \
   qpen \
   qpaintengine \
   qtransform \
   qwmatrix \
   qpolygon \

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
    qpathclipper \


