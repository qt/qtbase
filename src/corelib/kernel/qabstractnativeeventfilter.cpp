// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qabstractnativeeventfilter.h"
#include "qabstracteventdispatcher.h"

QT_BEGIN_NAMESPACE

/*!
    \class QAbstractNativeEventFilter
    \inmodule QtCore
    \since 5.0

    \brief The QAbstractNativeEventFilter class provides an interface for receiving native
    events, such as MSG or XCB event structs.
*/

/*!
    Creates a native event filter.

    By default this doesn't do anything. Remember to install it on the application
    object.
*/
QAbstractNativeEventFilter::QAbstractNativeEventFilter()
{
    Q_UNUSED(d);
}

/*!
    Destroys the native event filter.

    This automatically removes it from the application.
*/
QAbstractNativeEventFilter::~QAbstractNativeEventFilter()
{
    QAbstractEventDispatcher *eventDispatcher = QAbstractEventDispatcher::instance();
    if (eventDispatcher)
        eventDispatcher->removeNativeEventFilter(this);
}

/*!
    \fn bool QAbstractNativeEventFilter::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)

    This method is called for every native event.

    \note The filter function here receives native messages,
    for example, MSG or XCB event structs.

    It is called by the QPA platform plugin. On Windows, it is called by
    the event dispatcher.

    The type of event \a eventType is specific to the platform plugin chosen at run-time,
    and can be used to cast \a message to the right type.

    On X11, \a eventType is set to "xcb_generic_event_t", and the \a message can be casted
    to a xcb_generic_event_t pointer.

    On Windows, \a eventType is set to "windows_generic_MSG" for messages sent to toplevel windows,
    and "windows_dispatcher_MSG" for system-wide messages such as messages from a registered hot key.
    In both cases, the \a message can be casted to a MSG pointer.
    The \a result pointer is only used on Windows, and corresponds to the LRESULT pointer.

    On macOS, \a eventType is set to "mac_generic_NSEvent", and the \a message can be casted to an NSEvent pointer.

    In your reimplementation of this function, if you want to filter
    the \a message out, i.e. stop it being handled further, return
    true; otherwise return false.

    \b {Linux example}
    \snippet code/src_corelib_kernel_qabstractnativeeventfilter.cpp 0

    \b {Windows example}
    \snippet code/src_corelib_kernel_qabstractnativeeventfilter_win.cpp 0

    \b {macOS example}

    mycocoaeventfilter.h:
    \snippet code/src_corelib_kernel_qabstractnativeeventfilter.h 0

    mycocoaeventfilter.mm:
    \snippet code/src_corelib_kernel_qabstractnativeeventfilter.mm 0

    myapp.pro:
    \snippet code/src_corelib_kernel_qabstractnativeeventfilter.pro 0
*/

QT_END_NAMESPACE
