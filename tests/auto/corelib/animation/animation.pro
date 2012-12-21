TEMPLATE=subdirs
SUBDIRS=\
   qabstractanimation \
   qanimationgroup \
   qparallelanimationgroup \
   qpauseanimation \
   qpropertyanimation \
   qsequentialanimationgroup \
   qvariantanimation

!qtHaveModule(widgets): SUBDIRS -= \
   qpropertyanimation
