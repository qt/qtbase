TEMPLATE=subdirs
SUBDIRS=\
    qcoreapplication \
    qeventdispatcher \
    qeventloop \
    qmath \
    qmetaobject \
    qmetaobjectbuilder \
    qmetamethod \
    qmetaproperty \
    qmetatype \
    qmimedata \
    qobject \
    qpointer \
    qsharedmemory \
    qsignalmapper \
    qsocketnotifier \
    qsystemsemaphore \
    qtimer \
    qtranslator \
    qvariant \
    qwineventnotifier

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
    qsocketnotifier \
    qsharedmemory

# This test is only applicable on Windows
!win32*:SUBDIRS -= qwineventnotifier

