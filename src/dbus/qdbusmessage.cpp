// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qdbusmessage.h"
#include "qdbusmessage_p.h"

#include <qdebug.h>
#include <qstringlist.h>

#include "qdbus_symbols_p.h"

#include "qdbusargument_p.h"
#include "qdbuserror.h"
#include "qdbusmetatype.h"
#include "qdbusconnection_p.h"
#include "qdbusutil_p.h"

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QT_IMPL_METATYPE_EXTERN(QDBusMessage)

static_assert(QDBusMessage::InvalidMessage == DBUS_MESSAGE_TYPE_INVALID);
static_assert(QDBusMessage::MethodCallMessage == DBUS_MESSAGE_TYPE_METHOD_CALL);
static_assert(QDBusMessage::ReplyMessage == DBUS_MESSAGE_TYPE_METHOD_RETURN);
static_assert(QDBusMessage::ErrorMessage == DBUS_MESSAGE_TYPE_ERROR);
static_assert(QDBusMessage::SignalMessage == DBUS_MESSAGE_TYPE_SIGNAL);

static inline const char *data(const QByteArray &arr)
{
    return arr.isEmpty() ? nullptr : arr.constData();
}

QDBusMessagePrivate::QDBusMessagePrivate()
    : localReply(nullptr), ref(1), type(QDBusMessage::InvalidMessage),
      delayedReply(false), parametersValidated(false),
      localMessage(false), autoStartService(true),
      interactiveAuthorizationAllowed(false), isReplyRequired(false)
{
}

QDBusMessagePrivate::~QDBusMessagePrivate()
{
    delete localReply;
}

void QDBusMessagePrivate::createResponseLink(const QDBusMessagePrivate *call)
{
    if (Q_UNLIKELY(call->type != QDBusMessage::MethodCallMessage)) {
        qWarning("QDBusMessage: replying to a message that isn't a method call");
        return;
    }

    if (call->localMessage) {
        localMessage = true;
        call->localReply = new QDBusMessage(*this); // keep an internal copy
    } else {
        serial = call->serial;
        service = call->service;
    }

    // the reply must have a serial or be a local-loop optimization
    Q_ASSERT(serial || localMessage);
}

/*!
    \since 4.3
     Returns the human-readable message associated with the error that was received.
*/
QString QDBusMessage::errorMessage() const
{
    if (d_ptr->type == ErrorMessage) {
        if (!d_ptr->message.isEmpty())
           return d_ptr->message;
        if (!d_ptr->arguments.isEmpty())
            return d_ptr->arguments.at(0).toString();
    }
    return QString();
}

/*!
    \internal
    Constructs a DBusMessage object from \a message. The returned value must be de-referenced
    with q_dbus_message_unref. The \a capabilities flags indicates which capabilities to use.

    The \a error object is set to indicate the error if anything went wrong with the
    marshalling. Usually, this error message will be placed in the reply, as if the call failed.
    The \a error pointer must not be null.
*/
DBusMessage *QDBusMessagePrivate::toDBusMessage(const QDBusMessage &message, QDBusConnection::ConnectionCapabilities capabilities,
                                                QDBusError *error)
{
    if (!qdbus_loadLibDBus()) {
        *error = QDBusError(QDBusError::Failed, "Could not open lidbus-1 library"_L1);
        return nullptr;
    }

    DBusMessage *msg = nullptr;
    const QDBusMessagePrivate *d_ptr = message.d_ptr;

    switch (d_ptr->type) {
    case QDBusMessage::InvalidMessage:
        //qDebug() << "QDBusMessagePrivate::toDBusMessage" <<  "message is invalid";
        break;
    case QDBusMessage::MethodCallMessage:
        // only service and interface can be empty -> path and name must not be empty
        if (!d_ptr->parametersValidated) {
            using namespace QDBusUtil;
            AllowEmptyFlag serviceCheckMode = capabilities & QDBusConnectionPrivate::ConnectionIsBus
                    ? EmptyNotAllowed : EmptyAllowed;
            if (!checkBusName(d_ptr->service, serviceCheckMode, error))
                return nullptr;
            if (!QDBusUtil::checkObjectPath(d_ptr->path, QDBusUtil::EmptyNotAllowed, error))
                return nullptr;
            if (!QDBusUtil::checkInterfaceName(d_ptr->interface, QDBusUtil::EmptyAllowed, error))
                return nullptr;
            if (!QDBusUtil::checkMemberName(d_ptr->name, QDBusUtil::EmptyNotAllowed, error, "method"))
                return nullptr;
        }

        msg = q_dbus_message_new_method_call(data(d_ptr->service.toUtf8()), d_ptr->path.toUtf8(),
                                             data(d_ptr->interface.toUtf8()), d_ptr->name.toUtf8());
        q_dbus_message_set_auto_start( msg, d_ptr->autoStartService );
        q_dbus_message_set_allow_interactive_authorization(msg, d_ptr->interactiveAuthorizationAllowed);

        break;
    case QDBusMessage::ReplyMessage:
        msg = q_dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_RETURN);
        if (!d_ptr->localMessage) {
            q_dbus_message_set_destination(msg, data(d_ptr->service.toUtf8()));
            q_dbus_message_set_reply_serial(msg, d_ptr->serial);
        }
        break;
    case QDBusMessage::ErrorMessage:
        // error name can't be empty
        if (!d_ptr->parametersValidated
            && !QDBusUtil::checkErrorName(d_ptr->name, QDBusUtil::EmptyNotAllowed, error))
            return nullptr;

        msg = q_dbus_message_new(DBUS_MESSAGE_TYPE_ERROR);
        q_dbus_message_set_error_name(msg, d_ptr->name.toUtf8());
        if (!d_ptr->localMessage) {
            q_dbus_message_set_destination(msg, data(d_ptr->service.toUtf8()));
            q_dbus_message_set_reply_serial(msg, d_ptr->serial);
        }
        break;
    case QDBusMessage::SignalMessage:
        // only the service name can be empty here (even for bus connections)
        if (!d_ptr->parametersValidated) {
            if (!QDBusUtil::checkBusName(d_ptr->service, QDBusUtil::EmptyAllowed, error))
                return nullptr;
            if (!QDBusUtil::checkObjectPath(d_ptr->path, QDBusUtil::EmptyNotAllowed, error))
                return nullptr;
            if (!QDBusUtil::checkInterfaceName(d_ptr->interface, QDBusUtil::EmptyAllowed, error))
                return nullptr;
            if (!QDBusUtil::checkMemberName(d_ptr->name, QDBusUtil::EmptyNotAllowed, error, "method"))
                return nullptr;
        }

        msg = q_dbus_message_new_signal(d_ptr->path.toUtf8(), d_ptr->interface.toUtf8(),
                                        d_ptr->name.toUtf8());
        q_dbus_message_set_destination(msg, data(d_ptr->service.toUtf8()));
        break;
    }

    // if we got here, the parameters validated
    // and since the message parameters cannot be changed once the message is created
    // we can record this fact
    d_ptr->parametersValidated = true;

    QDBusMarshaller marshaller(capabilities);
    q_dbus_message_iter_init_append(msg, &marshaller.iterator);
    if (!d_ptr->message.isEmpty())
        // prepend the error message
        marshaller.append(d_ptr->message);
    for (const QVariant &argument : std::as_const(d_ptr->arguments))
        marshaller.appendVariantInternal(argument);

    // check if everything is ok
    if (marshaller.ok)
        return msg;

    // not ok;
    q_dbus_message_unref(msg);
    *error = QDBusError(QDBusError::Failed, "Marshalling failed: "_L1 + marshaller.errorString);
    return nullptr;
}

/*
struct DBusMessage
{
    DBusAtomic refcount;
    DBusHeader header;
    DBusString body;
    char byte_order;
    unsigned int locked : 1;
DBUS_DISABLE_CHECKS
    unsigned int in_cache : 1;
#endif
    DBusList *size_counters;
    long size_counter_delta;
    dbus_uint32_t changed_stamp : CHANGED_STAMP_BITS;
    DBusDataSlotList slot_list;
#ifndef DBUS_DISABLE_CHECKS
    int generation;
#endif
};
*/

/*!
    \internal
    Constructs a QDBusMessage by parsing the given DBusMessage object.
*/
QDBusMessage QDBusMessagePrivate::fromDBusMessage(DBusMessage *dmsg, QDBusConnection::ConnectionCapabilities capabilities)
{
    QDBusMessage message;
    if (!dmsg)
        return message;

    message.d_ptr->type = QDBusMessage::MessageType(q_dbus_message_get_type(dmsg));
    message.d_ptr->serial = q_dbus_message_get_serial(dmsg);
    message.d_ptr->path = QString::fromUtf8(q_dbus_message_get_path(dmsg));
    message.d_ptr->interface = QString::fromUtf8(q_dbus_message_get_interface(dmsg));
    message.d_ptr->name = message.d_ptr->type == DBUS_MESSAGE_TYPE_ERROR ?
                      QString::fromUtf8(q_dbus_message_get_error_name(dmsg)) :
                      QString::fromUtf8(q_dbus_message_get_member(dmsg));
    message.d_ptr->service = QString::fromUtf8(q_dbus_message_get_sender(dmsg));
    message.d_ptr->signature = QString::fromUtf8(q_dbus_message_get_signature(dmsg));
    message.d_ptr->interactiveAuthorizationAllowed = q_dbus_message_get_allow_interactive_authorization(dmsg);
    message.d_ptr->isReplyRequired = !q_dbus_message_get_no_reply(dmsg);

    QDBusDemarshaller demarshaller(capabilities);
    demarshaller.message = q_dbus_message_ref(dmsg);
    if (q_dbus_message_iter_init(demarshaller.message, &demarshaller.iterator))
        while (!demarshaller.atEnd())
            message << demarshaller.toVariantInternal();
    return message;
}

bool QDBusMessagePrivate::isLocal(const QDBusMessage &message)
{
    return message.d_ptr->localMessage;
}

QDBusMessage QDBusMessagePrivate::makeLocal(const QDBusConnectionPrivate &conn,
                                            const QDBusMessage &asSent)
{
    // simulate the message being sent to the bus and then received back
    // the only field that the bus sets when delivering the message
    // (as opposed to the message as we send it), is the sender
    // so we simply set the sender to our unique name

    // determine if we are carrying any complex types
    QString computedSignature;
    for (const QVariant &argument : std::as_const(asSent.d_ptr->arguments)) {
        QMetaType id = argument.metaType();
        const char *signature = QDBusMetaType::typeToSignature(id);
        if ((id.id() != QMetaType::QStringList && id.id() != QMetaType::QByteArray &&
             qstrlen(signature) != 1) || id == QMetaType::fromType<QDBusVariant>()) {
            // yes, we are
            // we must marshall and demarshall again so as to create QDBusArgument
            // entries for the complex types
            QDBusError error;
            DBusMessage *message = toDBusMessage(asSent, conn.connectionCapabilities(), &error);
            if (!message) {
                // failed to marshall, so it's a call error
                return QDBusMessage::createError(error);
            }

            q_dbus_message_set_sender(message, conn.baseService.toUtf8());

            QDBusMessage retval = fromDBusMessage(message, conn.connectionCapabilities());
            retval.d_ptr->localMessage = true;
            q_dbus_message_unref(message);
            if (retval.d_ptr->service.isEmpty())
                retval.d_ptr->service = conn.baseService;
            return retval;
        } else {
            computedSignature += QLatin1StringView(signature);
        }
    }

    // no complex types seen
    // optimize by using the variant list itself
    QDBusMessage retval;
    QDBusMessagePrivate *d = retval.d_ptr;
    d->arguments = asSent.d_ptr->arguments;
    d->path = asSent.d_ptr->path;
    d->interface = asSent.d_ptr->interface;
    d->name = asSent.d_ptr->name;
    d->message = asSent.d_ptr->message;
    d->type = asSent.d_ptr->type;

    d->service = conn.baseService;
    d->signature = computedSignature;
    d->localMessage = true;
    return retval;
}

QDBusMessage QDBusMessagePrivate::makeLocalReply(const QDBusConnectionPrivate &conn,
                                                 const QDBusMessage &callMsg)
{
    // simulate the reply (return or error) message being sent to the bus and
    // then received back.
    if (callMsg.d_ptr->localReply)
        return makeLocal(conn, *callMsg.d_ptr->localReply);
    return QDBusMessage();      // failed
}

/*!
    \class QDBusMessage
    \inmodule QtDBus
    \since 4.2

    \brief The QDBusMessage class represents one message sent or
    received over the D-Bus bus.

    This object can represent any of the four different types of
    messages (MessageType) that can occur on the bus:

    \list
      \li Method calls
      \li Method return values
      \li Signal emissions
      \li Error codes
    \endlist

    Objects of this type are created with the static createError(),
    createMethodCall() and createSignal() functions. Use the
    QDBusConnection::send() function to send the messages.
*/

/*!
    \enum QDBusMessage::MessageType
    The possible message types:

    \value MethodCallMessage    a message representing an outgoing or incoming method call
    \value SignalMessage        a message representing an outgoing or incoming signal emission
    \value ReplyMessage         a message representing the return values of a method call
    \value ErrorMessage         a message representing an error condition in response to a method call
    \value InvalidMessage       an invalid message: this is never set on messages received from D-Bus
*/

/*!
    Constructs a new DBus message with the given \a path, \a interface
    and \a name, representing a signal emission.

    A DBus signal is emitted from one application and is received by
    all applications that are listening for that signal from that
    interface.

    The QDBusMessage object that is returned can be sent using the
    QDBusConnection::send() function.
*/
QDBusMessage QDBusMessage::createSignal(const QString &path, const QString &interface,
                                        const QString &name)
{
    QDBusMessage message;
    message.d_ptr->type = SignalMessage;
    message.d_ptr->path = path;
    message.d_ptr->interface = interface;
    message.d_ptr->name = name;

    return message;
}

/*!
    \since 5.6

    Constructs a new DBus message with the given \a path, \a interface
    and \a name, representing a signal emission to a specific destination.

    A DBus signal is emitted from one application and is received only by
    the application owning the destination \a service name.

    The QDBusMessage object that is returned can be sent using the
    QDBusConnection::send() function.
*/
QDBusMessage QDBusMessage::createTargetedSignal(const QString &service, const QString &path,
                                                const QString &interface, const QString &name)
{
    QDBusMessage message;
    message.d_ptr->type = SignalMessage;
    message.d_ptr->service = service;
    message.d_ptr->path = path;
    message.d_ptr->interface = interface;
    message.d_ptr->name = name;

    return message;
}

/*!
    Constructs a new DBus message representing a method call.
    A method call always informs its destination address
    (\a service, \a path, \a interface and \a method).

    The DBus bus allows calling a method on a given remote object without specifying the
    destination interface, if the method name is unique. However, if two interfaces on the
    remote object export the same method name, the result is undefined (one of the two may be
    called or an error may be returned).

    When using DBus in a peer-to-peer context (i.e., not on a bus), the \a service parameter is
    optional.

    The QDBusInterface class provides a simpler abstraction to synchronous
    method calling.

    This function returns a QDBusMessage object that can be sent with
    QDBusConnection::call().
*/
QDBusMessage QDBusMessage::createMethodCall(const QString &service, const QString &path,
                                            const QString &interface, const QString &method)
{
    QDBusMessage message;
    message.d_ptr->type = MethodCallMessage;
    message.d_ptr->service = service;
    message.d_ptr->path = path;
    message.d_ptr->interface = interface;
    message.d_ptr->name = method;
    message.d_ptr->isReplyRequired = true;

    return message;
}

/*!
    Constructs a new DBus message representing an error,
    with the given \a name and \a msg.
*/
QDBusMessage QDBusMessage::createError(const QString &name, const QString &msg)
{
    QDBusMessage error;
    error.d_ptr->type = ErrorMessage;
    error.d_ptr->name = name;
    error.d_ptr->message = msg;

    return error;
}

/*!
    \fn QDBusMessage QDBusMessage::createError(const QDBusError &error)

    Constructs a new DBus message representing the given \a error.
*/

/*!
  \fn QDBusMessage QDBusMessage::createError(QDBusError::ErrorType type, const QString &msg)

  Constructs a new DBus message for the error type \a type using
  the message \a msg. Returns the DBus message.
*/

/*!
    \fn QDBusMessage QDBusMessage::createReply(const QList<QVariant> &arguments) const

    Constructs a new DBus message representing a reply, with the given
    \a arguments.
*/
QDBusMessage QDBusMessage::createReply(const QVariantList &arguments) const
{
    QDBusMessage reply;
    reply.setArguments(arguments);
    reply.d_ptr->type = ReplyMessage;
    reply.d_ptr->createResponseLink(d_ptr);
    return reply;
}

/*!
    Constructs a new DBus message representing an error reply message,
    with the given \a name and \a msg.
*/
QDBusMessage QDBusMessage::createErrorReply(const QString &name, const QString &msg) const
{
    QDBusMessage reply = QDBusMessage::createError(name, msg);
    reply.d_ptr->createResponseLink(d_ptr);
    return reply;
}

/*!
    Constructs a new DBus message representing a reply, with the
    given \a argument.
*/
QDBusMessage QDBusMessage::createReply(const QVariant &argument) const
{
    return createReply(QList{argument});
}

/*!
    \fn QDBusMessage QDBusMessage::createErrorReply(const QDBusError &error) const

    Constructs a new DBus message representing an error reply message,
    from the given \a error object.
*/

/*!
  \fn QDBusMessage QDBusMessage::createErrorReply(QDBusError::ErrorType type, const QString &msg) const

  Constructs a new DBus reply message for the error type \a type using
  the message \a msg. Returns the DBus message.
*/
QDBusMessage QDBusMessage::createErrorReply(QDBusError::ErrorType atype, const QString &amsg) const
{
    QDBusMessage msg = createErrorReply(QDBusError::errorString(atype), amsg);
    msg.d_ptr->parametersValidated = true;
    return msg;
}


/*!
    Constructs an empty, invalid QDBusMessage object.

    \sa createError(), createMethodCall(), createSignal()
*/
QDBusMessage::QDBusMessage()
{
    d_ptr = new QDBusMessagePrivate;
}

/*!
    Constructs a copy of the object given by \a other.

    Note: QDBusMessage objects are shared. Modifications made to the
    copy will affect the original one as well. See setDelayedReply()
    for more information.
*/
QDBusMessage::QDBusMessage(const QDBusMessage &other)
{
    d_ptr = other.d_ptr;
    d_ptr->ref.ref();
}

/*!
    Disposes of the object and frees any resources that were being held.
*/
QDBusMessage::~QDBusMessage()
{
    if (!d_ptr->ref.deref())
        delete d_ptr;
}

/*!
    \fn QDBusMessage &QDBusMessage::operator=(QDBusMessage &&other)

    Move-assigns \a other into this object.

//! [partially-formed]
    \note The moved-from object \a other is placed in a partially-formed state,
    in which the only valid operations are destruction and assignment of a new
    value.
//! [partially-formed]
*/

/*!
    Copies the contents of the object given by \a other.

    Note: QDBusMessage objects are shared. Modifications made to the
    copy will affect the original one as well. See setDelayedReply()
    for more information.
*/
QDBusMessage &QDBusMessage::operator=(const QDBusMessage &other)
{
    qAtomicAssign(d_ptr, other.d_ptr);
    return *this;
}

/*!
    Returns the name of the service or the bus address of the remote method call.
*/
QString QDBusMessage::service() const
{
    if (d_ptr->type == ErrorMessage || d_ptr->type == ReplyMessage)
        return QString();   // d_ptr->service holds the destination
    return d_ptr->service;
}

/*!
    Returns the path of the object that this message is being sent to (in the case of a
    method call) or being received from (for a signal).
*/
QString QDBusMessage::path() const
{
    return d_ptr->path;
}

/*!
    Returns the interface of the method being called (in the case of a method call) or of
    the signal being received from.
*/
QString QDBusMessage::interface() const
{
    return d_ptr->interface;
}

/*!
    Returns the name of the signal that was emitted or the name of the method that was called.
*/
QString QDBusMessage::member() const
{
    if (d_ptr->type != ErrorMessage)
        return d_ptr->name;
    return QString();
}

/*!
    Returns the name of the error that was received.
*/
QString QDBusMessage::errorName() const
{
    if (d_ptr->type == ErrorMessage)
        return d_ptr->name;
    return QString();
}

/*!
    Returns the signature of the signal that was received or for the output arguments
    of a method call.
*/
QString QDBusMessage::signature() const
{
    return d_ptr->signature;
}

/*!
    Returns the flag that indicates if this message should see a reply
    or not. This is only meaningful for \l {MethodCallMessage}{method
    call messages}: any other kind of message cannot have replies and
    this function will always return false for them.
*/
bool QDBusMessage::isReplyRequired() const
{
    // Only method calls can have replies
    if (d_ptr->type != QDBusMessage::MethodCallMessage)
        return false;

    if (d_ptr->localMessage) // if it's a local message, reply is required
        return true;
    return d_ptr->isReplyRequired;
}

/*!
    Sets whether the message will be replied later (if \a enable is
    true) or if an automatic reply should be generated by Qt D-Bus
    (if \a enable is false).

    In D-Bus, all method calls must generate a reply to the caller, unless the
    caller explicitly indicates otherwise (see isReplyRequired()). QtDBus
    automatically generates such replies for any slots being called, but it
    also allows slots to indicate whether they will take responsibility
    of sending the reply at a later time, after the function has finished
    processing.

    \sa {Delayed Replies}
*/
void QDBusMessage::setDelayedReply(bool enable) const
{
    d_ptr->delayedReply = enable;
}

/*!
    Returns the delayed reply flag, as set by setDelayedReply(). By default, this
    flag is false, which means Qt D-Bus will generate automatic replies
    when necessary.
*/
bool QDBusMessage::isDelayedReply() const
{
    return d_ptr->delayedReply;
}

/*!
    Sets the auto start flag to \a enable. This flag only makes sense
    for method call messages, where it tells the D-Bus server to
    either auto start the service responsible for the service name, or
    not to auto start it.

    By default this flag is true, i.e. a service is autostarted.
    This means:

    When the service that this method call is sent to is already
    running, the method call is sent to it. If the service is not
    running yet, the D-Bus daemon is requested to autostart the
    service that is assigned to this service name. This is
    handled by .service files that are placed in a directory known
    to the D-Bus server. These files then each contain a service
    name and the path to a program that should be executed when
    this service name is requested.

    \since 4.7
*/
void QDBusMessage::setAutoStartService(bool enable)
{
    d_ptr->autoStartService = enable;
}

/*!
    Returns the auto start flag, as set by setAutoStartService(). By default, this
    flag is true, which means Qt D-Bus will auto start a service, if it is
    not running already.

    \sa setAutoStartService()

    \since 4.7
*/
bool QDBusMessage::autoStartService() const
{
    return d_ptr->autoStartService;
}

/*!
    Enables or disables the \c ALLOW_INTERACTIVE_AUTHORIZATION flag
    in a message.

    This flag only makes sense for method call messages
    (\l QDBusMessage::MethodCallMessage). If \a enable
    is set to \c true, the flag indicates to the callee that the
    caller of the method is prepared to wait for interactive authorization
    to take place (for instance via Polkit) before the actual method
    is processed.

    If \a enable is set to \c false, the flag is not
    set, meaning that the other end is expected to make any authorization
    decisions non-interactively and promptly. This is the default.

    The \c org.freedesktop.DBus.Error.InteractiveAuthorizationRequired
    error indicates that authorization failed, but could have succeeded
    if this flag had been set.

    \sa isInteractiveAuthorizationAllowed(),
        QDBusAbstractInterface::setInteractiveAuthorizationAllowed()

    \since 5.12
*/
void QDBusMessage::setInteractiveAuthorizationAllowed(bool enable)
{
    d_ptr->interactiveAuthorizationAllowed = enable;
}

/*!
    Returns whether the message has the
    \c ALLOW_INTERACTIVE_AUTHORIZATION flag set.

    \sa setInteractiveAuthorizationAllowed(),
        QDBusAbstractInterface::isInteractiveAuthorizationAllowed()

    \since 5.12
*/
bool QDBusMessage::isInteractiveAuthorizationAllowed() const
{
    return d_ptr->interactiveAuthorizationAllowed;
}

/*!
    Sets the arguments that are going to be sent over D-Bus to \a arguments. Those
    will be the arguments to a method call or the parameters in the signal.

    Note that QVariantMap with invalid QVariant as value is not allowed
    in \a arguments.

    \sa arguments()
*/
void QDBusMessage::setArguments(const QList<QVariant> &arguments)
{
    d_ptr->arguments = arguments;
}

/*!
    Returns the list of arguments that are going to be sent or were received from
    D-Bus.
*/
QList<QVariant> QDBusMessage::arguments() const
{
    return d_ptr->arguments;
}

/*!
    Appends the argument \a arg to the list of arguments to be sent over D-Bus in
    a method call or signal emission.
*/

QDBusMessage &QDBusMessage::operator<<(const QVariant &arg)
{
    d_ptr->arguments.append(arg);
    return *this;
}

QDBusMessage::QDBusMessage(QDBusMessagePrivate &dd)
    : d_ptr(&dd)
{
    d_ptr->ref.ref();
}

/*!
    Returns the message type.
*/
QDBusMessage::MessageType QDBusMessage::type() const
{
    switch (d_ptr->type) {
    case DBUS_MESSAGE_TYPE_METHOD_CALL:
        return MethodCallMessage;
    case DBUS_MESSAGE_TYPE_METHOD_RETURN:
        return ReplyMessage;
    case DBUS_MESSAGE_TYPE_ERROR:
        return ErrorMessage;
    case DBUS_MESSAGE_TYPE_SIGNAL:
        return SignalMessage;
    default:
        break;
    }
    return InvalidMessage;
}

#ifndef QT_NO_DEBUG_STREAM
static QDebug operator<<(QDebug dbg, QDBusMessage::MessageType t)
{
    switch (t)
    {
    case QDBusMessage::MethodCallMessage:
        return dbg << "MethodCall";
    case QDBusMessage::ReplyMessage:
        return dbg << "MethodReturn";
    case QDBusMessage::SignalMessage:
        return dbg << "Signal";
    case QDBusMessage::ErrorMessage:
        return dbg << "Error";
    default:
        return dbg << "Invalid";
    }
}

static void debugVariantList(QDebug dbg, const QVariantList &list)
{
    bool first = true;
    for (const QVariant &elem : list) {
        if (!first)
            dbg.nospace() << ", ";
        dbg.nospace() << qPrintable(QDBusUtil::argumentToString(elem));
        first = false;
    }
}

QDebug operator<<(QDebug dbg, const QDBusMessage &msg)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QDBusMessage(type=" << msg.type()
                  << ", service=" << msg.service();
    if (msg.type() == QDBusMessage::MethodCallMessage ||
        msg.type() == QDBusMessage::SignalMessage)
        dbg.nospace() << ", path=" << msg.path()
                      << ", interface=" << msg.interface()
                      << ", member=" << msg.member();
    if (msg.type() == QDBusMessage::ErrorMessage)
        dbg.nospace() << ", error name=" << msg.errorName()
                      << ", error message=" << msg.errorMessage();
    dbg.nospace() << ", signature=" << msg.signature()
                  << ", contents=(";
    debugVariantList(dbg, msg.arguments());
    dbg.nospace() << ") )";
    return dbg;
}
#endif

/*!
    \fn void QDBusMessage::swap(QDBusMessage &other)
    \memberswap{message}
*/

QT_END_NAMESPACE

#endif // QT_NO_DBUS
