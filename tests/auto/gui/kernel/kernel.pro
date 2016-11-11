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
   qkeyevent \
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
   qopenglwindow \
   qrasterwindow

win32:!winrt:qtHaveModule(network): SUBDIRS += noqteventloop

!qtHaveModule(widgets): SUBDIRS -= \
   qmouseevent_modal \
   qtouchevent

!qtHaveModule(network): SUBDIRS -= \
   qguieventloop

!qtConfig(opengl): SUBDIRS -= qopenglwindow

uikit: SUBDIRS -= qclipboard
