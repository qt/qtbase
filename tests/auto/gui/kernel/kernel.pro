TEMPLATE=subdirs
SUBDIRS=\
   qbackingstore \
   qclipboard \
   qdrag \
   qevent \
   qfileopenevent \
   qguieventdispatcher \
   qguimetatype \
   qguitimer \
   qguivariant \
   qinputmethod \
   qkeysequence \
   qmouseevent \
   qmouseevent_modal \
   qpalette \
   qscreen \
   qsurfaceformat \
   qtouchevent \
   qwindow \
   qguiapplication \

!qtHaveModule(widgets): SUBDIRS -= \
   qmouseevent_modal \
   qtouchevent
