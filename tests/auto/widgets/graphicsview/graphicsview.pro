TEMPLATE=subdirs
SUBDIRS=\
   qgraphicsanchorlayout \
   qgraphicsanchorlayout1 \
   qgraphicseffectsource \
   qgraphicsgridlayout \
   qgraphicsitem \
   qgraphicsitemanimation \
   qgraphicslayout \
   qgraphicslayoutitem \
   qgraphicslinearlayout \
   qgraphicsobject \
   qgraphicspixmapitem \
   qgraphicspolygonitem \
   qgraphicsproxywidget \
   qgraphicsscene \
   qgraphicssceneindex \
   qgraphicstransform \
   qgraphicsview \
   qgraphicswidget \

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
           qgraphicsanchorlayout \
           qgraphicsanchorlayout1 \
           qgraphicsitem \
           qgraphicsscene \
           qgraphicssceneindex \

# These tests require the cleanlooks style
!contains(styles, cleanlooks):SUBDIRS -= \
    qgraphicsproxywidget \
    qgraphicswidget \
