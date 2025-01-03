// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdbuspendingreply.h"
#include "qdbuspendingcall_p.h"
#include "qdbusmetatype.h"

#include <QtCore/private/qlocking_p.h>

#ifndef QT_NO_DBUS

/*!
    \class QDBusPendingReply
    \inmodule QtDBus
    \since 4.5

    \brief The QDBusPendingReply class contains the reply to an asynchronous method call.

    The QDBusPendingReply is a variadic template class. The template parameters
    are the types that will be used to extract the contents of the reply's data.

    This class is similar in functionality to QDBusReply, but with two
    important differences:

    \list
      \li QDBusReply accepts exactly one return type, whereas
         QDBusPendingReply can have any number of types
      \li QDBusReply only works on already completed replies, whereas
         QDBusPendingReply allows one to wait for replies from pending
         calls
    \endlist

    Where with QDBusReply you would write:

    \snippet code/src_qdbus_qdbusreply.cpp 0

    with QDBusPendingReply, the equivalent code (including the blocking
    wait for the reply) would be:

    \snippet code/src_qdbus_qdbuspendingreply.cpp 0

    For method calls that have more than one output argument, with
    QDBusReply, you would write:

    \snippet code/src_qdbus_qdbusreply.cpp 1

    whereas with QDBusPendingReply, all of the output arguments should
    be template parameters:

    \snippet code/src_qdbus_qdbuspendingreply.cpp 2

    QDBusPendingReply objects can be associated with
    QDBusPendingCallWatcher objects, which emit signals when the reply
    arrives.

    \sa QDBusPendingCallWatcher, QDBusReply
*/

/*!
    \fn template<typename... Types> QDBusPendingReply<Types...>::QDBusPendingReply()

    Creates an empty QDBusPendingReply object. Without assigning a
    QDBusPendingCall object to this reply, QDBusPendingReply cannot do
    anything. All functions return their failure values.
*/

/*!
    \fn template<typename... Types> QDBusPendingReply<Types...>::QDBusPendingReply(const QDBusPendingReply &other)

    Creates a copy of the \a other QDBusPendingReply object. Just like
    QDBusPendingCall and QDBusPendingCallWatcher, this QDBusPendingReply
    object will share the same pending call reference. All copies
    share the same return values.
*/

/*!
    \fn template<typename... Types> QDBusPendingReply<Types...>::QDBusPendingReply(const QDBusPendingCall &call)

    Creates a QDBusPendingReply object that will take its contents from
    the \a call pending asynchronous call. This QDBusPendingReply object
    will share the same pending call reference as \a call.
*/

/*!
    \fn template<typename... Types> QDBusPendingReply<Types...>::QDBusPendingReply(const QDBusMessage &message)

    Creates a QDBusPendingReply object that will take its contents from
    the message \a message. In this case, this object will be already
    in its finished state and the reply's contents will be accessible.

    \sa isFinished()
*/

/*!
    \fn template<typename... Types> QDBusPendingReply &QDBusPendingReply<Types...>::operator=(const QDBusPendingReply &other)

    Makes a copy of \a other and drops the reference to the current
    pending call. If the current reference is to an unfinished pending
    call and this is the last reference, the pending call will be
    canceled and there will be no way of retrieving the reply's
    contents, when they arrive.
*/

/*!
    \fn template<typename... Types> QDBusPendingReply &QDBusPendingReply<Types...>::operator=(const QDBusPendingCall &call)

    Makes this object take its contents from the \a call pending call
    and drops the reference to the current pending call. If the
    current reference is to an unfinished pending call and this is the
    last reference, the pending call will be canceled and there will
    be no way of retrieving the reply's contents, when they arrive.
*/

/*!
    \fn template<typename... Types> QDBusPendingReply &QDBusPendingReply<Types...>::operator=(const QDBusMessage &message)

    Makes this object take its contents from the \a message message
    and drops the reference to the current pending call. If the
    current reference is to an unfinished pending call and this is the
    last reference, the pending call will be canceled and there will
    be no way of retrieving the reply's contents, when they arrive.

    After this function is finished, the QDBusPendingReply object will
    be in its "finished" state and the \a message contents will be
    accessible.

    \sa isFinished()
*/

/*!
  \enum QDBusPendingReply::anonymous

  \value Count The number of arguments the reply is expected to have
 */

/*!
    \fn template<typename... Types> int QDBusPendingReply<Types...>::count() const

    Return the number of arguments the reply is supposed to have. This
    number matches the number of non-void template parameters in this
    class.

    If the reply arrives with a different number of arguments (or with
    different types), it will be transformed into an error reply
    indicating a bad signature.
*/

/*!
    \fn template<typename... Types> QVariant QDBusPendingReply<Types...>::argumentAt(int index) const

    Returns the argument at position \a index in the reply's
    contents. If the reply doesn't have that many elements, this
    function's return value is undefined (will probably cause an
    assertion failure), so it is important to verify that the
    processing is finished and the reply is valid.

    If the reply does not contain an argument at position \a index or if the
    reply was an error, this function returns an invalid QVariant. Since D-Bus
    messages can never contain invalid QVariants, this return can be used to
    detect an error condition.
*/

/*!
    \fn template<typename... Types> typename Select<0>::Type QDBusPendingReply<Types...>::value() const

    Returns the first argument in this reply, cast to type \c Types[0] (the
    first template parameter of this class). This is equivalent to
    calling argumentAt<0>().

    This function is provided as a convenience, matching the
    QDBusReply::value() function.

    Note that, if the reply hasn't arrived, this function causes the
    calling thread to block until the reply is processed.

    If the reply is an error reply, this function returns a default-constructed
    \c Types[0] object, which may be indistinguishable from a valid value. To
    reliably determine whether the message was an error, use isError().
*/

/*!
    \fn template<typename... Types> QDBusPendingReply<Types...>::operator typename Select<0>::Type() const

    Returns the first argument in this reply, cast to type \c Types[0] (the
    first template parameter of this class). This is equivalent to
    calling argumentAt<0>().

    This function is provided as a convenience, matching the
    QDBusReply::value() function.

    Note that, if the reply hasn't arrived, this function causes the
    calling thread to block until the reply is processed.

    If the reply is an error reply, this function returns a default-constructed
    \c Types[0] object, which may be indistinguishable from a valid value. To
    reliably determine whether the message was an error, use isError().
*/

/*!
    \fn template<typename... Types> void QDBusPendingReply<Types...>::waitForFinished()

    Suspends the execution of the calling thread until the reply is
    received and processed. After this function returns, isFinished()
    should return true, indicating the reply's contents are ready to
    be processed.

    \sa QDBusPendingCallWatcher::waitForFinished()
*/

QDBusPendingReplyBase::QDBusPendingReplyBase()
    : QDBusPendingCall(nullptr)         // initialize base class empty
{
}

QDBusPendingReplyBase::~QDBusPendingReplyBase()
{
}

void QDBusPendingReplyBase::assign(const QDBusPendingCall &other)
{
    QDBusPendingCall::operator=(other);
}

void QDBusPendingReplyBase::assign(const QDBusMessage &message)
{
    d = new QDBusPendingCallPrivate(QDBusMessage(), nullptr); // drops the reference to the old one
    d->replyMessage = message;
}

QVariant QDBusPendingReplyBase::argumentAt(int index) const
{
    if (!d)
        return QVariant();

    d->waitForFinished();   // bypasses "const"

    return d->replyMessage.arguments().value(index);
}

void QDBusPendingReplyBase::setMetaTypes(int count, const QMetaType *types)
{
    Q_ASSERT(d);
    const auto locker = qt_scoped_lock(d->mutex);
    d->setMetaTypes(count, types);
    d->checkReceivedSignature();
}

#endif // QT_NO_DBUS
