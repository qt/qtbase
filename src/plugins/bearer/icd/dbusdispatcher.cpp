/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QDebug>
#include <QtCore>
#include <poll.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <glib.h>
#include "dbusdispatcher.h"

namespace Maemo {

/*!
    \class Maemo::DBusDispatcher

    \brief DBusDispatcher is a class that can send DBUS method call
    messages and receive unicast signals from DBUS objects.
*/

class DBusDispatcherPrivate
{
public:
    DBusDispatcherPrivate(const QString& service,
                          const QString& path,
                          const QString& interface,
                          const QString& signalPath)
        : service(service), path(path), interface(interface),
          signalPath(signalPath), connection(0)
    {
        memset(&signal_vtable, 0, sizeof(signal_vtable));
    }

    ~DBusDispatcherPrivate()
    {
        foreach(DBusPendingCall *call, pending_calls) {
            dbus_pending_call_cancel(call);
            dbus_pending_call_unref(call);
        }
    }

    QString service;
    QString path;
    QString interface;
    QString signalPath;
    struct DBusConnection *connection;
    QList<DBusPendingCall *> pending_calls;
    struct DBusObjectPathVTable signal_vtable;
};

static bool constantVariantList(const QVariantList& variantList) {
    // Special case, empty list == empty struct
    if (variantList.isEmpty()) {
        return false;
    } else {        
        QVariant::Type type = variantList[0].type();
        // Iterate items in the list and check if they are same type
        foreach(QVariant variant, variantList) {
            if (variant.type() != type) {
                return false;
            }
        }
    }
    return true;
}

static QString variantToSignature(const QVariant& argument,
                                  bool constantList = true) {
    switch (argument.type()) {
        case QVariant::Bool:
            return "b";
        case QVariant::ByteArray:
            return "ay";
        case QVariant::Char:
            return "y";
        case QVariant::Int:
            return "i";
        case QVariant::UInt:
            return "u";
        case QVariant::StringList:
            return "as";
        case QVariant::String:
            return "s";
        case QVariant::LongLong:
            return "x";
        case QVariant::ULongLong:
            return "t";
        case QVariant::List:
            {
            QString signature;
            QVariantList variantList = argument.toList();
            if (!constantList) {
                signature += DBUS_STRUCT_BEGIN_CHAR_AS_STRING;
                foreach(QVariant listItem, variantList) {
                    signature += variantToSignature(listItem);
                }
                signature += DBUS_STRUCT_END_CHAR_AS_STRING;
            } else {
                if (variantList.isEmpty())
                    return "";
                signature = "a" + variantToSignature(variantList[0]);
            }

            return signature;
            }
        default:
            qDebug() << "Unsupported variant type: " << argument.type();
            break;
    }

    return "";
}

static bool appendVariantToDBusMessage(const QVariant& argument,
                                       DBusMessageIter *dbus_iter) {
    int idx = 0;
    DBusMessageIter array_iter;
    QStringList str_list;
    dbus_bool_t bool_data;
    dbus_int32_t int32_data;
    dbus_uint32_t uint32_data;
    dbus_int64_t int64_data;
    dbus_uint64_t uint64_data;
    char *str_data;
    char char_data;

    switch (argument.type()) {

        case QVariant::Bool:
            bool_data = argument.toBool();
            dbus_message_iter_append_basic(dbus_iter, DBUS_TYPE_BOOLEAN, 
                                           &bool_data);
            break;

        case QVariant::ByteArray:
            str_data = argument.toByteArray().data();
            dbus_message_iter_open_container(dbus_iter, DBUS_TYPE_ARRAY,
                                             DBUS_TYPE_BYTE_AS_STRING, &array_iter);
            dbus_message_iter_append_fixed_array(&array_iter,
                                                 DBUS_TYPE_BYTE,
                                                 &str_data,
                                                 argument.toByteArray().size());
            dbus_message_iter_close_container(dbus_iter, &array_iter);
            break;

        case QVariant::Char:
            char_data = argument.toChar().toAscii();
            dbus_message_iter_append_basic(dbus_iter, DBUS_TYPE_BYTE, 
                                           &char_data);
            break;

        case QVariant::Int:
            int32_data = argument.toInt();
            dbus_message_iter_append_basic(dbus_iter, DBUS_TYPE_INT32, 
                                           &int32_data);
            break;

        case QVariant::String: {
            QByteArray data = argument.toString().toLatin1();
            str_data = data.data();
            dbus_message_iter_append_basic(dbus_iter, DBUS_TYPE_STRING,
                                           &str_data);
            break;
        }

        case QVariant::StringList:
            str_list = argument.toStringList();
            dbus_message_iter_open_container(dbus_iter, DBUS_TYPE_ARRAY,
                                             "s", &array_iter);
            for (idx = 0; idx < str_list.size(); idx++) {
                QByteArray data = str_list.at(idx).toLatin1();
                str_data = data.data();
                dbus_message_iter_append_basic(&array_iter,
                                               DBUS_TYPE_STRING,
                                               &str_data);
            }
            dbus_message_iter_close_container(dbus_iter, &array_iter);
            break;

        case QVariant::UInt:
            uint32_data = argument.toUInt();
            dbus_message_iter_append_basic(dbus_iter, DBUS_TYPE_UINT32, 
                                           &uint32_data);
            break;

        case QVariant::ULongLong:
            uint64_data = argument.toULongLong();
            dbus_message_iter_append_basic(dbus_iter, DBUS_TYPE_UINT64, 
                                           &uint64_data);
            break;

        case QVariant::LongLong:
            int64_data = argument.toLongLong();
            dbus_message_iter_append_basic(dbus_iter, DBUS_TYPE_INT64, 
                                           &int64_data);
            break;

        case QVariant::List:
            {
            QVariantList variantList = argument.toList();
            bool constantList = constantVariantList(variantList);
            DBusMessageIter array_iter;

            // List is mapped either as an DBUS array (all items same type)
            // DBUS struct (variable types) depending on constantList
            if (constantList) {
                // Resolve the signature for the first item
                QString signature = "";
                if (!variantList.isEmpty()) {
                    signature = variantToSignature(
                                variantList[0],
                                constantVariantList(variantList[0].toList()));
                }

                // Mapped as DBUS array
                dbus_message_iter_open_container(dbus_iter, DBUS_TYPE_ARRAY,
                                                 signature.toAscii(),
                                                 &array_iter);

                foreach(QVariant listItem, variantList) {
                    appendVariantToDBusMessage(listItem, &array_iter);
                }

                dbus_message_iter_close_container(dbus_iter, &array_iter);
            } else {
                // Mapped as DBUS struct
                dbus_message_iter_open_container(dbus_iter, DBUS_TYPE_STRUCT,
                                                 NULL,
                                                 &array_iter);

                foreach(QVariant listItem, variantList) {
                    appendVariantToDBusMessage(listItem, &array_iter);
                }

                dbus_message_iter_close_container(dbus_iter, &array_iter);
            }

            break;
            }
        default:
            qDebug() << "Unsupported variant type: " << argument.type();
            break;
    }

    return true;
}

static QVariant getVariantFromDBusMessage(DBusMessageIter *iter) {
    dbus_bool_t bool_data;
    dbus_int32_t int32_data;
    dbus_uint32_t uint32_data;
    dbus_int64_t int64_data;
    dbus_uint64_t uint64_data;
    char *str_data;
    char char_data;
    int argtype = dbus_message_iter_get_arg_type(iter);

    switch (argtype) {

        case DBUS_TYPE_BOOLEAN:
        {
            dbus_message_iter_get_basic(iter, &bool_data);
            QVariant variant((bool)bool_data);
            return variant;
        }

        case DBUS_TYPE_ARRAY:
        {
            // Handle all arrays here
            int elem_type = dbus_message_iter_get_element_type(iter);
            DBusMessageIter array_iter;

            dbus_message_iter_recurse(iter, &array_iter);

            if (elem_type == DBUS_TYPE_BYTE) {
                QByteArray byte_array;
                do {
                    dbus_message_iter_get_basic(&array_iter, &char_data);
                    byte_array.append(char_data);
                } while (dbus_message_iter_next(&array_iter));
                QVariant variant(byte_array);
                return variant;
            } else if (elem_type == DBUS_TYPE_STRING) {
                QStringList str_list;
                do {
                    dbus_message_iter_get_basic(&array_iter, &str_data);
                    str_list.append(str_data);
                } while (dbus_message_iter_next(&array_iter));
                QVariant variant(str_list);
                return variant;
            } else {
                QVariantList variantList;
                do {
                    variantList << getVariantFromDBusMessage(&array_iter);
                } while (dbus_message_iter_next(&array_iter));
                QVariant variant(variantList);
                return variant;
            }
            break;
        }

        case DBUS_TYPE_BYTE:
        {
            dbus_message_iter_get_basic(iter, &char_data);
            QChar ch(char_data);
            QVariant variant(ch);
            return variant;
        }

        case DBUS_TYPE_INT32:
        {
            dbus_message_iter_get_basic(iter, &int32_data);
            QVariant variant((int)int32_data);
            return variant;
        }

        case DBUS_TYPE_UINT32:
        {
            dbus_message_iter_get_basic(iter, &uint32_data);
            QVariant variant((uint)uint32_data);
            return variant;
        }

        case DBUS_TYPE_STRING:
        {
            dbus_message_iter_get_basic(iter, &str_data);
            QString str(str_data);
            QVariant variant(str);
            return variant;
        }

        case DBUS_TYPE_INT64:
        {
            dbus_message_iter_get_basic(iter, &int64_data);
            QVariant variant((qlonglong)int64_data);
            return variant;
        }

        case DBUS_TYPE_UINT64:
        {
            dbus_message_iter_get_basic(iter, &uint64_data);
            QVariant variant((qulonglong)uint64_data);
            return variant;
        }

        case DBUS_TYPE_STRUCT:
        {
            // Handle all structs here
            DBusMessageIter struct_iter;
            dbus_message_iter_recurse(iter, &struct_iter);

	    QVariantList variantList;
	    do {
	      variantList << getVariantFromDBusMessage(&struct_iter);
	    } while (dbus_message_iter_next(&struct_iter));
	    QVariant variant(variantList);
	    return variant;
        }

        default:
            qDebug() << "Unsupported DBUS type: " << argtype;
    }

    return QVariant();
}

static DBusHandlerResult signalHandler (DBusConnection *connection,
                                        DBusMessage *message,
                                        void *object_ref) {
    (void)connection;
    QString interface;
    QString signal;
    DBusDispatcher *dispatcher = (DBusDispatcher *)object_ref;

    if (dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_SIGNAL) {
        interface = dbus_message_get_interface(message);
        signal = dbus_message_get_member(message);
        
        QList<QVariant> arglist;
        DBusMessageIter dbus_iter;

        if (dbus_message_iter_init(message, &dbus_iter)) {
            // Read return arguments
            while (dbus_message_iter_get_arg_type (&dbus_iter) != DBUS_TYPE_INVALID) {
                arglist << getVariantFromDBusMessage(&dbus_iter);
                dbus_message_iter_next(&dbus_iter);
            }
        }

        dispatcher->emitSignalReceived(interface, signal, arglist);
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    (void)message;
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

DBusDispatcher::DBusDispatcher(const QString& service,
                               const QString& path,
                               const QString& interface,
                               QObject *parent)
 : QObject(parent),
   d_ptr(new DBusDispatcherPrivate(service, path, interface, path)) {
    setupDBus();
}

DBusDispatcher::DBusDispatcher(const QString& service,
                               const QString& path,
                               const QString& interface,
                               const QString& signalPath,
                               QObject *parent)
 : QObject(parent),
   d_ptr(new DBusDispatcherPrivate(service, path, interface, signalPath)) {
    setupDBus();
}

DBusDispatcher::~DBusDispatcher()
{
    if (d_ptr->connection) {
        dbus_connection_close(d_ptr->connection);
        dbus_connection_unref(d_ptr->connection);
    }
    delete d_ptr;
}

void DBusDispatcher::setupDBus()
{
    d_ptr->connection = dbus_bus_get_private(DBUS_BUS_SYSTEM, NULL);

    if (d_ptr->connection == NULL)
        qDebug() << "Unable to get DBUS connection!";
    else {
        d_ptr->signal_vtable.message_function = signalHandler;

	dbus_connection_set_exit_on_disconnect(d_ptr->connection, FALSE);
        dbus_connection_setup_with_g_main(d_ptr->connection, g_main_context_get_thread_default());
        dbus_connection_register_object_path(d_ptr->connection, 
                                             d_ptr->signalPath.toLatin1(),
                                             &d_ptr->signal_vtable,
                                             this);
    }
}

static DBusMessage *prepareDBusCall(const QString& service,
                                    const QString& path,
                                    const QString& interface,
                                    const QString& method, 
                                    const QVariant& arg1 = QVariant(),
                                    const QVariant& arg2 = QVariant(),
                                    const QVariant& arg3 = QVariant(),
                                    const QVariant& arg4 = QVariant(),
                                    const QVariant& arg5 = QVariant(),
                                    const QVariant& arg6 = QVariant(),
                                    const QVariant& arg7 = QVariant(),
                                    const QVariant& arg8 = QVariant()) 
{
    DBusMessage *message = dbus_message_new_method_call(service.toLatin1(),
                                                        path.toLatin1(),
                                                        interface.toLatin1(),
                                                        method.toLatin1());
    DBusMessageIter dbus_iter;

    // Append variants to DBUS message
    QList<QVariant> arglist;
    if (arg1.isValid()) arglist << arg1;
    if (arg2.isValid()) arglist << arg2;
    if (arg3.isValid()) arglist << arg3;
    if (arg4.isValid()) arglist << arg4;
    if (arg5.isValid()) arglist << arg5;
    if (arg6.isValid()) arglist << arg6;
    if (arg7.isValid()) arglist << arg7;
    if (arg8.isValid()) arglist << arg8;

    dbus_message_iter_init_append (message, &dbus_iter);
    
    while (!arglist.isEmpty()) {
        QVariant argument = arglist.takeFirst();
        appendVariantToDBusMessage(argument, &dbus_iter);
    }

    return message;
}

QList<QVariant> DBusDispatcher::call(const QString& method, 
                                     const QVariant& arg1,
                                     const QVariant& arg2,
                                     const QVariant& arg3,
                                     const QVariant& arg4,
                                     const QVariant& arg5,
                                     const QVariant& arg6,
                                     const QVariant& arg7,
                                     const QVariant& arg8) {
    DBusMessageIter dbus_iter;
    DBusMessage *message = prepareDBusCall(d_ptr->service, d_ptr->path,
                                           d_ptr->interface, method,
                                           arg1, arg2, arg3, arg4, arg5,
                                           arg6, arg7, arg8);
    DBusMessage *reply = dbus_connection_send_with_reply_and_block(
                                                    d_ptr->connection,
                                                    message, -1, NULL);
    dbus_message_unref(message);

    QList<QVariant> replylist;
    if (reply != NULL &&  dbus_message_iter_init(reply, &dbus_iter)) {
        // Read return arguments
        while (dbus_message_iter_get_arg_type (&dbus_iter) != DBUS_TYPE_INVALID) {
            replylist << getVariantFromDBusMessage(&dbus_iter);
            dbus_message_iter_next(&dbus_iter);
        }
    }
    if (reply != NULL) dbus_message_unref(reply);
    return replylist;
}

class PendingCallInfo {
public:
    QString method;
    DBusDispatcher *dispatcher;
    DBusDispatcherPrivate *priv;
};

static void freePendingCallInfo(void *memory) {
    PendingCallInfo *info = (PendingCallInfo *)memory;
    delete info;
}

static void pendingCallFunction (DBusPendingCall *pending,
                                 void *memory) {
    PendingCallInfo *info = (PendingCallInfo *)memory;
    QString errorStr;
    QList<QVariant> replyList;
    DBusMessage *reply = dbus_pending_call_steal_reply (pending);

    Q_ASSERT(reply != NULL);

    if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
        errorStr = dbus_message_get_error_name (reply);
    } else {
        DBusMessageIter dbus_iter;
        dbus_message_iter_init(reply, &dbus_iter);
        // Read return arguments
        while (dbus_message_iter_get_arg_type (&dbus_iter) != DBUS_TYPE_INVALID) {
            replyList << getVariantFromDBusMessage(&dbus_iter);
            dbus_message_iter_next(&dbus_iter);
        }
    }

    info->priv->pending_calls.removeOne(pending);
    info->dispatcher->emitCallReply(info->method, replyList, errorStr);
    dbus_message_unref(reply);
    dbus_pending_call_unref(pending);
}

bool DBusDispatcher::callAsynchronous(const QString& method, 
                                      const QVariant& arg1,
                                      const QVariant& arg2,
                                      const QVariant& arg3,
                                      const QVariant& arg4,
                                      const QVariant& arg5,
                                      const QVariant& arg6,
                                      const QVariant& arg7,
                                      const QVariant& arg8) {
    DBusMessage *message = prepareDBusCall(d_ptr->service, d_ptr->path,
                                           d_ptr->interface, method,
                                           arg1, arg2, arg3, arg4, arg5,
                                           arg6, arg7, arg8);
    DBusPendingCall *call = NULL;
    dbus_bool_t ret = dbus_connection_send_with_reply(d_ptr->connection,
                                                      message, &call, -1);
    PendingCallInfo *info = new PendingCallInfo;
    info->method = method;
    info->dispatcher = this;
    info->priv = d_ptr;

    dbus_pending_call_set_notify(call, pendingCallFunction, info, freePendingCallInfo);
    d_ptr->pending_calls.append(call);
    return (bool)ret;
}

void DBusDispatcher::emitSignalReceived(const QString& interface, 
                                        const QString& signal,
                                        const QList<QVariant>& args) {
    emit signalReceived(interface, signal, args); }

void DBusDispatcher::emitCallReply(const QString& method,
                                   const QList<QVariant>& args,
                                   const QString& error) {
    emit callReply(method, args, error); }

void DBusDispatcher::synchronousDispatch(int timeout_ms)
{
    dbus_connection_read_write_dispatch(d_ptr->connection, timeout_ms);
}

} // Maemo namespace

