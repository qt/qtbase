TEMPLATE = subdirs
SUBDIRS = \
        events \
        qmetaobject \
        qmetatype \
        qobject \
        qvariant \
        qcoreapplication

!qtHaveModule(widgets): SUBDIRS -= \
    qmetaobject \
    qobject
