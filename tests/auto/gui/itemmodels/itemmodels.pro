TEMPLATE=subdirs
SUBDIRS= \
    qstandarditem \
    qstandarditemmodel

!qtHaveModule(widgets): SUBDIRS -= \
    qstandarditemmodel
