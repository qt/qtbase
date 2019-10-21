TEMPLATE = subdirs
SUBDIRS = \
    json \
    qcborstreamreader \
    qcborstreamwriter \
    qcborvalue \
    qcborvalue_json \
    qdatastream \
    qdatastream_core_pixmap \
    qtextstream \
    qxmlstream

!qtHaveModule(gui): SUBDIRS -= \
    qdatastream \
    qdatastream_core_pixmap

!qtHaveModule(network): SUBDIRS -= \
    qtextstream

!qtHaveModule(network)|!qtHaveModule(xml): SUBDIRS -= \
    qxmlstream
