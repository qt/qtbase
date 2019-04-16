TEMPLATE=subdirs
SUBDIRS=\
   qpainterpath \
   qpainterpathstroker \
   qcolor \
   qcolorspace \
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

!qtConfig(private_tests): SUBDIRS -= \
    qpathclipper \


