TEMPLATE=subdirs
SUBDIRS=\
    qabstractitemmodel \
    qcoreapplication \
    qeventloop \
    qitemmodel \
    qmath \
    qmetaobject \
    qmetaobjectbuilder \
    qmetatype \
    qmimedata \
    qobject \
    qpointer \
    qsignalmapper \
    qsocketnotifier \
    qtimer \
    qtranslator \
    qvariant \
    qwineventnotifier

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
    qsocketnotifier

# This test is only applicable on Windows
!win32*:SUBDIRS -= qwineventnotifier

mac: qabstractitemmodel.CONFIG = no_check_target # QTBUG-22748
