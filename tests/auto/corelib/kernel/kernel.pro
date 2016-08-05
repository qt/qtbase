TEMPLATE=subdirs
SUBDIRS=\
    qcoreapplication \
    qdeadlinetimer \
    qelapsedtimer \
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

!qtConfig(private_tests): SUBDIRS -= \
    qsocketnotifier \
    qsharedmemory

# This test is only applicable on Windows
!win32*|winrt: SUBDIRS -= qwineventnotifier

android|uikit: SUBDIRS -= qclipboard qobject qsharedmemory qsystemsemaphore
