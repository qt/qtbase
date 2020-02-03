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
   qpolygon \

!qtConfig(private_tests): SUBDIRS -= \
    qpathclipper \


