TEMPLATE=subdirs
SUBDIRS=\
   qaction \
   qactiongroup \
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
   qhighdpi\
   qinputdevice \
   qinputmethod \
   qkeyevent \
   qkeysequence \
   qmouseevent \
   qmouseevent_modal \
   qpalette \
   qscreen \
   qshortcut \
   qsurfaceformat \
   qtouchevent \
   qwindow \
   qguiapplication \
   qpixelformat \
   qopenglwindow \
   qrasterwindow \
   qaddpostroutine

win32:qtHaveModule(network): SUBDIRS += noqteventloop

!qtConfig(shortcut): SUBDIRS -= \
   qkeysequence \
   qshortcut \
   qguimetatype \
   qguivariant

!qtHaveModule(widgets): SUBDIRS -= \
   qmouseevent_modal \
   qtouchevent

!qtHaveModule(network): SUBDIRS -= \
   qguieventloop

!qtConfig(action): SUBDIRS -= \
   qaction \
   qactiongroup

!qtConfig(highdpiscaling): SUBDIRS -= qhighdpiscaling

!qtConfig(opengl): SUBDIRS -= qopenglwindow

android|uikit: SUBDIRS -= qclipboard
