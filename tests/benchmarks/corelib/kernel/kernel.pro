TEMPLATE = subdirs
SUBDIRS = \
        events \
        qmetaobject \
        qmetatype \
        qobject \
        qvariant \
        qcoreapplication

isEmpty(QT.widgets.name): SUBDIRS -= \
    qmetaobject \
    qobject
