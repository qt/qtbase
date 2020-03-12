TEMPLATE=subdirs
SUBDIRS=\
   qbackingstore \
   qclipboard \
   qcursor \
   qdrag \
   qevent \
   qfileopenevent \
   qguieventdispatcher \
   qguieventloop \
   qguimetatype \
   qguitimer \
   qguivariant \
   qhighdpiscaling \
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
   qrasterwindow \
   qaddpostroutine

win32:!winrt:qtHaveModule(network): SUBDIRS += noqteventloop

!qtHaveModule(widgets): SUBDIRS -= \
   qmouseevent_modal \
   qtouchevent

!qtHaveModule(network): SUBDIRS -= \
   qguieventloop

!qtConfig(highdpiscaling): SUBDIRS -= qhighdpiscaling

!qtConfig(opengl): SUBDIRS -= qopenglwindow

android|uikit: SUBDIRS -= qclipboard
