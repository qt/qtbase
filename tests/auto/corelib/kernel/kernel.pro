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
    qmetaenum \
    qmimedata \
    qobject \
    qpointer \
    qsharedmemory \
    qsignalblocker \
    qsignalmapper \
    qsocketnotifier \
    qsystemsemaphore \
    qtimer \
    qtranslator \
    qvariant \
    qwineventnotifier

!qtHaveModule(gui): SUBDIRS -= \
    qmimedata

!qtHaveModule(network): SUBDIRS -= \
    qeventloop \
    qobject \
    qsocketnotifier

!contains(QT_CONFIG, private_tests): SUBDIRS -= \
    qsocketnotifier \
    qsharedmemory

# This test is only applicable on Windows
!win32*|winrt: SUBDIRS -= qwineventnotifier

android|ios: SUBDIRS -= qsharedmemory qsystemsemaphore
