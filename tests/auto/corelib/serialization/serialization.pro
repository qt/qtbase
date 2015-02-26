TEMPLATE = subdirs
SUBDIRS = \
    json \
    qcborstreamwriter \
    qdatastream \
    qtextstream \
    qxmlstream

!qtHaveModule(gui): SUBDIRS -= \
    qdatastream

!qtHaveModule(network): SUBDIRS -= \
    qtextstream

!qtHaveModule(network)|!qtHaveModule(xml): SUBDIRS -= \
    qxmlstream
