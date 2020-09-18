TEMPLATE = subdirs
SUBDIRS = \
        events \
        qmetaobject \
        qmetatype \
        qobject \
        qvariant \
        qcoreapplication \
        qtimer_vs_qmetaobject \
        qwineventnotifier

!qtHaveModule(widgets): SUBDIRS -= \
    qmetaobject \
    qobject

# This test is only applicable on Windows
!win32: SUBDIRS -= qwineventnotifier
