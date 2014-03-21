TEMPLATE=subdirs
SUBDIRS=\
   qbackingstore \
   qclipboard \
   qdrag \
   qevent \
   qfileopenevent \
   qguieventdispatcher \
   qguieventloop \
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
   qpixelformat \

!qtHaveModule(widgets): SUBDIRS -= \
   qmouseevent_modal \
   qtouchevent

!qtHaveModule(network): SUBDIRS -= \
   qguieventloop
