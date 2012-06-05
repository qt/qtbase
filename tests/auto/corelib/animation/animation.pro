TEMPLATE=subdirs
SUBDIRS=\
   qabstractanimation \
   qanimationgroup \
   qparallelanimationgroup \
   qpauseanimation \
   qpropertyanimation \
   qsequentialanimationgroup \
   qvariantanimation

contains(QT_CONFIG, no-widgets): SUBDIRS -= \
   qpropertyanimation
