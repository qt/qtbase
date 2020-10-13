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
   qrangecollection \
   qtransform \
   qpolygon \

!qtConfig(private_tests): SUBDIRS -= \
    qpathclipper \

# QTBUG-87669
android: SUBDIRS -= \
            qcolorspace
