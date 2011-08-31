TEMPLATE=subdirs
SUBDIRS=\
   qclipboard \
   qdrag \
   qevent \
   qfileopenevent \
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
