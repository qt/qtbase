// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdbusreply.h"
#include "qdbusmetatype.h"
#include "qdbusmetatype_p.h"
#include <QDebug>

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    \class QDBusReply
    \inmodule QtDBus
    \since 4.2

    \brief The QDBusReply class stores the reply for a method call to a remote object.

    A QDBusReply object is a subset of the QDBusMessage object that represents a method call's
    reply. It contains only the first output argument or the error code and is used by
    QDBusInterface-derived classes to allow returning the error code as the function's return
    argument.

    It can be used in the following manner:
    \snippet code/src_qdbus_qdbusreply.cpp 0

    If the remote method call cannot fail, you can skip the error checking:
    \snippet code/src_qdbus_qdbusreply.cpp 1

    However, if it does fail under those conditions, the value returned by QDBusReply<T>::value() is
    a default-constructed value. It may be indistinguishable from a valid return value.

    QDBusReply objects are used for remote calls that have no output
    arguments or return values (i.e., they have a "void" return
    type). Use the isValid() function to test if the reply succeeded.

    \sa QDBusMessage, QDBusInterface
*/

/*!
    \fn template<typename T> QDBusReply<T>::QDBusReply(const QDBusReply &other)
    \since 5.15

    Constructs a copy of \a other.
*/

/*!
    \fn template<typename T> QDBusReply<T>::QDBusReply(const QDBusMessage &reply)
    Automatically construct a QDBusReply object from the reply message \a reply, extracting the
    first return value from it if it is a success reply.
*/

/*!
    \fn template<typename T> QDBusReply<T>::QDBusReply(const QDBusPendingReply<T> &reply)
    Constructs a QDBusReply object from the pending reply message, \a reply.
*/

/*!
    \fn template <typename T> QDBusReply<T>::QDBusReply(const QDBusPendingCall &pcall)
    Automatically construct a QDBusReply object from the asynchronous
    pending call \a pcall. If the call isn't finished yet, QDBusReply
    will call QDBusPendingCall::waitForFinished(), which is a blocking
    operation.

    If the return types patch, QDBusReply will extract the first
    return argument from the reply.
*/

/*!
    \fn template <typename T> QDBusReply<T>::QDBusReply(const QDBusError &error)
    Constructs an error reply from the D-Bus error code given by \a error.
*/

/*!
    \fn template <typename T> QDBusReply<T>::operator=(const QDBusReply &other)
    Makes this object be a copy of the object \a other.
*/

/*!
    \fn template <typename T> QDBusReply<T>::operator=(const QDBusError &dbusError)
    Sets this object to contain the error code given by \a dbusError. You
    can later access it with error().
*/

/*!
    \fn template <typename T> QDBusReply<T>::operator=(const QDBusMessage &reply)

    Makes this object contain the \a reply message. If \a reply
    is an error message, this function will
    copy the error code and message into this object

    If \a reply is a standard reply message and contains at least
    one parameter, it will be copied into this object, as long as it
    is of the correct type. If it's not of the same type as this
    QDBusError object, this function will instead set an error code
    indicating a type mismatch.
*/

/*!
    \fn template <typename T> QDBusReply<T>::operator=(const QDBusPendingCall &pcall)

    Makes this object contain the reply specified by the pending
    asynchronous call \a pcall. If the call is not finished yet, this
    function will call QDBusPendingCall::waitForFinished() to block
    until the reply arrives.

    If \a pcall finishes with an error message, this function will
    copy the error code and message into this object

    If \a pcall finished with a standard reply message and contains at
    least one parameter, it will be copied into this object, as long
    as it is of the correct type. If it's not of the same type as this
    QDBusError object, this function will instead set an error code
    indicating a type mismatch.
*/

/*!
    \fn template <typename T> bool QDBusReply<T>::isValid() const

    Returns \c true if no error occurred; otherwise, returns \c false.

    \sa error()
*/

/*!
    \fn template<typename T> const QDBusError& QDBusReply<T>::error() const

    Returns the error code that was returned from the remote function call. If the remote call did
    not return an error (i.e., if it succeeded), then the QDBusError object that is returned will
    not be a valid error code (QDBusError::isValid() will return false).

    \sa isValid()
*/

/*!
    \fn template <typename T> const QDBusError& QDBusReply<T>::error()
    \overload
*/

/*!
    \fn template <typename T> QDBusReply<T>::value() const
    Returns the remote function's calls return value. If the remote call returned with an error,
    the return value of this function is undefined and may be undistinguishable from a valid return
    value.

    This function is not available if the remote call returns \c void.
*/

/*!
    \fn template <typename T> QDBusReply<T>::operator Type() const
    Returns the same as value().

    This function is not available if the remote call returns \c void.
*/

/*!
    \internal
    Fills in the QDBusReply data \a error and \a data from the reply message \a reply.
*/
void qDBusReplyFill(const QDBusMessage &reply, QDBusError &error, QVariant &data)
{
    error = QDBusError(reply);

    if (error.isValid()) {
        data = QVariant();      // clear it
        return;
    }

    if (reply.arguments().size() >= 1 && reply.arguments().at(0).metaType() == data.metaType()) {
        data = reply.arguments().at(0);
        return;
    }

    const char *expectedSignature = QDBusMetaType::typeToSignature(data.metaType());
    const char *receivedType = nullptr;
    QByteArray receivedSignature;

    if (reply.arguments().size() >= 1) {
        if (reply.arguments().at(0).metaType() == QDBusMetaTypeId::argument()) {
            // compare signatures instead
            QDBusArgument arg = qvariant_cast<QDBusArgument>(reply.arguments().at(0));
            receivedSignature = arg.currentSignature().toLatin1();
            if (receivedSignature == expectedSignature) {
                // matched. Demarshall it
                QDBusMetaType::demarshall(arg, data.metaType(), data.data());
                return;
            }
        } else {
            // not an argument and doesn't match?
            QMetaType type = reply.arguments().at(0).metaType();
            receivedType = type.name();
            receivedSignature = QDBusMetaType::typeToSignature(type);
        }
    }

    // error
    if (receivedSignature.isEmpty())
        receivedSignature = "<empty signature>";
    QString errorMsg;
    if (receivedType) {
        errorMsg = "Unexpected reply signature: got \"%1\" (%4), expected \"%2\" (%3)"_L1
                   .arg(QLatin1StringView(receivedSignature),
                        QLatin1StringView(expectedSignature),
                        QLatin1StringView(data.typeName()),
                        QLatin1StringView(receivedType));
    } else {
        errorMsg = "Unexpected reply signature: got \"%1\", expected \"%2\" (%3)"_L1
                   .arg(QLatin1StringView(receivedSignature),
                        QLatin1StringView(expectedSignature),
                        QLatin1StringView(data.typeName()));
    }

    error = QDBusError(QDBusError::InvalidSignature, errorMsg);
    data = QVariant();      // clear it
}

QT_END_NAMESPACE

#endif // QT_NO_DBUS
