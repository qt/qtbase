TEMPLATE=subdirs
SUBDIRS= \
    qstandarditem \
    qstandarditemmodel \
    qfilesystemmodel

mingw: SUBDIRS -= qfilesystemmodel # QTBUG-29403

!qtHaveModule(widgets): SUBDIRS -= \
    qstandarditemmodel
