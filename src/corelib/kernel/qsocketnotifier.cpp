// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#define BUILDING_QSOCKETNOTIFIER
#include "qsocketnotifier.h"
#undef BUILDING_QSOCKETNOTIFIER

#include "qplatformdefs.h"

#include "qabstracteventdispatcher.h"
#include "qcoreapplication.h"

#include "qmetatype.h"

#include "qobject_p.h"
#include <private/qthread_p.h>

#include <QtCore/QLoggingCategory>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcSocketNotifierDeprecation)
Q_LOGGING_CATEGORY(lcSocketNotifierDeprecation, "qt.core.socketnotifier_deprecation");

QT_IMPL_METATYPE_EXTERN_TAGGED(QSocketNotifier::Type, QSocketNotifier_Type)
QT_IMPL_METATYPE_EXTERN(QSocketDescriptor)

class QSocketNotifierPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QSocketNotifier)
public:
    QSocketDescriptor sockfd;
    QSocketNotifier::Type sntype;
    bool snenabled = false;
};

/*!
    \class QSocketNotifier
    \inmodule QtCore
    \brief The QSocketNotifier class provides support for monitoring
    activity on a file descriptor.

    \ingroup network
    \ingroup io

    The QSocketNotifier makes it possible to integrate Qt's event
    loop with other event loops based on file descriptors. File
    descriptor action is detected in Qt's main event
    loop (QCoreApplication::exec()).

    \target write notifiers

    Once you have opened a device using a low-level (usually
    platform-specific) API, you can create a socket notifier to
    monitor the file descriptor. If the descriptor is passed to the
    notifier's constructor, the socket notifier is enabled by default,
    i.e. it emits the activated() signal whenever a socket event
    corresponding to its type occurs. Connect the activated() signal
    to the slot you want to be called when an event corresponding to
    your socket notifier's type occurs.

    You can create a socket notifier with no descriptor assigned. In
    this case, you should call the setSocket() function after the
    descriptor has been obtained.

    There are three types of socket notifiers: read, write, and
    exception. The type is described by the \l Type enum, and must be
    specified when constructing the socket notifier. After
    construction it can be determined using the type() function. Note
    that if you need to monitor both reads and writes for the same
    file descriptor, you must create two socket notifiers. Note also
    that it is not possible to install two socket notifiers of the
    same type (\l Read, \l Write, \l Exception) on the same socket.

    The setEnabled() function allows you to disable as well as enable
    the socket notifier. It is generally advisable to explicitly
    enable or disable the socket notifier, especially for write
    notifiers. A disabled notifier ignores socket events (the same
    effect as not creating the socket notifier). Use the isEnabled()
    function to determine the notifier's current status.

    Finally, you can use the socket() function to retrieve the
    socket identifier.  Although the class is called QSocketNotifier,
    it is normally used for other types of devices than sockets.
    QTcpSocket and QUdpSocket provide notification through signals, so
    there is normally no need to use a QSocketNotifier on them.

    \sa QFile, QProcess, QTcpSocket, QUdpSocket
*/

/*!
    \enum QSocketNotifier::Type

    This enum describes the various types of events that a socket
    notifier can recognize. The type must be specified when
    constructing the socket notifier.

    Note that if you need to monitor both reads and writes for the
    same file descriptor, you must create two socket notifiers. Note
    also that it is not possible to install two socket notifiers of
    the same type (Read, Write, Exception) on the same socket.

    \value Read      There is data to be read.
    \value Write      Data can be written.
    \value Exception  An exception has occurred. We recommend against using this.

    \sa QSocketNotifier(), type()
*/

/*!
    \since 6.1

    Constructs a socket notifier with the given \a type that has no
    descriptor assigned. The \a parent argument is passed to QObject's
    constructor.

    Call the setSocket() function to set the descriptor for monitoring.

    \sa setSocket(), isValid(), isEnabled()
*/

QSocketNotifier::QSocketNotifier(Type type, QObject *parent)
    : QObject(*new QSocketNotifierPrivate, parent)
{
    Q_D(QSocketNotifier);

    qRegisterMetaType<QSocketDescriptor>();
    qRegisterMetaType<QSocketNotifier::Type>();

    d->sntype = type;
}

/*!
    Constructs a socket notifier with the given \a parent. It enables
    the \a socket, and watches for events of the given \a type.

    It is generally advisable to explicitly enable or disable the
    socket notifier, especially for write notifiers.

    \b{Note for Windows users:} The socket passed to QSocketNotifier
    will become non-blocking, even if it was created as a blocking socket.

    \sa setEnabled(), isEnabled()
*/

QSocketNotifier::QSocketNotifier(qintptr socket, Type type, QObject *parent)
    : QSocketNotifier(type, parent)
{
    Q_D(QSocketNotifier);

    d->sockfd = socket;
    d->snenabled = true;

    auto thisThreadData = d->threadData.loadRelaxed();

    if (!d->sockfd.isValid())
        qWarning("QSocketNotifier: Invalid socket specified");
    else if (!thisThreadData->hasEventDispatcher())
        qWarning("QSocketNotifier: Can only be used with threads started with QThread");
    else
        thisThreadData->eventDispatcher.loadRelaxed()->registerSocketNotifier(this);
}

/*!
    Destroys this socket notifier.
*/

QSocketNotifier::~QSocketNotifier()
{
    setEnabled(false);
}


/*!
    \fn void QSocketNotifier::activated(int socket)
    \deprecated To avoid unintended truncation of the descriptor, use
    the QSocketDescriptor overload of this function. If you need
    compatibility with versions older than 5.15 you need to change
    the slot to accept qintptr if it currently accepts an int, and
    then connect using Functor-Based Connection.

    This signal is emitted whenever the socket notifier is enabled and
    a socket event corresponding to its \l {Type}{type} occurs.

    The socket identifier is passed in the \a socket parameter.

    \sa type(), socket()
*/

/*!
    \fn void QSocketNotifier::activated(QSocketDescriptor socket, QSocketNotifier::Type type)
    \since 5.15

    This signal is emitted whenever the socket notifier is enabled and
    a socket event corresponding to its \a type occurs.

    The socket identifier is passed in the \a socket parameter.

    \sa type(), socket()
*/

/*!
    \since 6.1

    Assigns the \a socket to this notifier.

    \note The notifier will be disabled as a side effect and needs
    to be re-enabled.

    \sa setEnabled(), isValid()
*/
void QSocketNotifier::setSocket(qintptr socket)
{
    Q_D(QSocketNotifier);

    setEnabled(false);
    d->sockfd = socket;
}

/*!
    Returns the socket identifier assigned to this object.

    \sa isValid(), type()
*/
qintptr QSocketNotifier::socket() const
{
    Q_D(const QSocketNotifier);
    return qintptr(d->sockfd);
}

/*!
    Returns the socket event type specified to the constructor.

    \sa socket()
*/
QSocketNotifier::Type QSocketNotifier::type() const
{
    Q_D(const QSocketNotifier);
    return d->sntype;
}

/*!
    \since 6.1

    Returns \c true if the notifier is valid (that is, it has
    a descriptor assigned); otherwise returns \c false.

    \sa setSocket()
*/
bool QSocketNotifier::isValid() const
{
    Q_D(const QSocketNotifier);
    return d->sockfd.isValid();
}

/*!
    Returns \c true if the notifier is enabled; otherwise returns \c false.

    \sa setEnabled()
*/
bool QSocketNotifier::isEnabled() const
{
    Q_D(const QSocketNotifier);
    return d->snenabled;
}

/*!
    If \a enable is true, the notifier is enabled; otherwise the notifier
    is disabled.

    When the notifier is enabled, it emits the activated() signal whenever
    a socket event corresponding to its \l{type()}{type} occurs. When it is
    disabled, it ignores socket events (the same effect as not creating
    the socket notifier).

    Write notifiers should normally be disabled immediately after the
    activated() signal has been emitted

    \sa isEnabled(), activated()
*/

void QSocketNotifier::setEnabled(bool enable)
{
    Q_D(QSocketNotifier);
    if (!d->sockfd.isValid())
        return;
    if (d->snenabled == enable)                        // no change
        return;
    d->snenabled = enable;


    auto thisThreadData = d->threadData.loadRelaxed();

    if (!thisThreadData->hasEventDispatcher()) // perhaps application/thread is shutting down
        return;
    if (Q_UNLIKELY(thread() != QThread::currentThread())) {
        qWarning("QSocketNotifier: Socket notifiers cannot be enabled or disabled from another thread");
        return;
    }
    if (d->snenabled)
        thisThreadData->eventDispatcher.loadRelaxed()->registerSocketNotifier(this);
    else
        thisThreadData->eventDispatcher.loadRelaxed()->unregisterSocketNotifier(this);
}


/*!\reimp
*/
bool QSocketNotifier::event(QEvent *e)
{
    Q_D(QSocketNotifier);
    // Emits the activated() signal when a QEvent::SockAct or QEvent::SockClose is
    // received.
    switch (e->type()) {
    case QEvent::ThreadChange:
        if (d->snenabled) {
            QMetaObject::invokeMethod(this, "setEnabled", Qt::QueuedConnection,
                                      Q_ARG(bool, d->snenabled));
            setEnabled(false);
        }
        break;
    case QEvent::SockAct:
    case QEvent::SockClose:
        {
            QPointer<QSocketNotifier> alive(this);
            emit activated(d->sockfd, d->sntype, QPrivateSignal());
            // ### Qt7: Remove emission if the activated(int) signal is removed
            if (alive)
                emit activated(int(qintptr(d->sockfd)), QPrivateSignal());
        }
        return true;
    default:
        break;
    }
    return QObject::event(e);
}

/*!
    \class QSocketDescriptor
    \inmodule QtCore
    \brief A class which holds a native socket descriptor.
    \internal

    \ingroup network
    \ingroup io

    \since 5.15

    QSocketDescriptor makes it easier to handle native socket
    descriptors in cross-platform code.

    On Windows it holds a \c {Qt::HANDLE} and on Unix it holds an \c int.
    The class will implicitly convert between the class and the
    native descriptor type.
*/

/*!
    \fn QSocketDescriptor::QSocketDescriptor(DescriptorType descriptor)
    \internal

    Construct a QSocketDescriptor from a native socket \a descriptor.
*/

/*!
    \fn QSocketDescriptor::QSocketDescriptor(qintptr descriptor)
    \internal

    Construct a QSocketDescriptor from a native socket \a descriptor.

    \note This constructor is only available on Windows.
*/

/*!
    \fn Qt::HANDLE QSocketDescriptor::winHandle() const noexcept
    \internal

    Returns the internal handle.

    \note This function is only available on Windows.
*/

QT_END_NAMESPACE

#include "moc_qsocketnotifier.cpp"
