TEMPLATE=subdirs
SUBDIRS=\
    qabstractitemmodel \
    qcoreapplication \
    qeventloop \
    qitemmodel \
    qmath \
    qmetaobject \
    qmetatype \
    qmimedata \
    qobject \
    qpointer \
    qsignalmapper \
    qsocketnotifier \
    qtimer \
    # qtipc \ # needs to be moved to qtscript
    qtranslator \
    qvariant \
    qwineventnotifier

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
    qsocketnotifier

# This test is only applicable on Windows
!win32*:SUBDIRS -= qwineventnotifier
