TEMPLATE=subdirs
SUBDIRS=\
   qclipboard \
   qdrag \
   qevent \
   qfileopenevent \
   qinputpanel \
   qguivariant \
   qkeysequence \
   qmouseevent \
   qmouseevent_modal \
   qpalette \
   qshortcut \
   qtouchevent \

symbian {
    SUBDIRS += qsoftkeymanager
}
