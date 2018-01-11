TEMPLATE = subdirs
SUBDIRS = \
    json \
    qcborstreamreader \
    qcborstreamwriter \
    qcborvalue \
    qcborvalue_json \
    qdatastream \
    qtextstream \
    qxmlstream

!qtHaveModule(gui): SUBDIRS -= \
    qdatastream

!qtHaveModule(network): SUBDIRS -= \
    qtextstream

!qtHaveModule(network)|!qtHaveModule(xml): SUBDIRS -= \
    qxmlstream
