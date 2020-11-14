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

# QTBUG-87671
android: SUBDIRS -= qgraphicsview

!qtConfig(private_tests): SUBDIRS -= \
           qgraphicsanchorlayout \
           qgraphicsanchorlayout1 \
           qgraphicsitem \
           qgraphicsscene \
           qgraphicssceneindex \

# These tests require the fusion style
!contains(styles, fusion):SUBDIRS -= \
    qgraphicsproxywidget \
    qgraphicswidget \
