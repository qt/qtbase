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

# Building the qmetatype test with Clang on ARM takes forever (QTBUG-37237)
!clang|!contains(QT_ARCH, arm): \
    SUBDIRS += qmetatype

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

android|qnx: SUBDIRS -= qsharedmemory qsystemsemaphore
