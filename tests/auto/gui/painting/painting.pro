TEMPLATE=subdirs
SUBDIRS=\
   qpainterpath \
   qpainterpathstroker \
   qcolor \
   qcolorspace \
   qbrush \
   qregion \
   qpagelayout \
   qpageranges \
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

# QTBUG-87669
android: SUBDIRS -= \
            qcolorspace
