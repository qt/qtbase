TEMPLATE = subdirs
SUBDIRS = \
        events \
        qmetaobject \
        qmetatype \
        qobject \
        qvariant \
        qcoreapplication \
        qtimer_vs_qmetaobject

!qtHaveModule(widgets): SUBDIRS -= \
    qmetaobject \
    qobject
