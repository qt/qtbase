// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qdbusargument_p.h"
#include "qdbusconnection.h"
#include "qdbusmetatype_p.h"
#include "qdbusutil_p.h"

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static void qIterAppend(DBusMessageIter *it, QByteArray *ba, int type, const void *arg)
{
    if (ba)
        *ba += char(type);
    else
        q_dbus_message_iter_append_basic(it, type, arg);
}

QDBusMarshaller::~QDBusMarshaller()
{
    close();
}

void QDBusMarshaller::unregisteredTypeError(QMetaType id)
{
    const char *name = id.name();
    qWarning("QDBusMarshaller: type '%s' (%d) is not registered with D-Bus. "
             "Use qDBusRegisterMetaType to register it",
             name ? name : "", id.id());
    error("Unregistered type %1 passed in arguments"_L1
          .arg(QLatin1StringView(id.name())));
}

inline QString QDBusMarshaller::currentSignature()
{
    if (message)
        return QString::fromUtf8(q_dbus_message_get_signature(message));
    return QString();
}

inline void QDBusMarshaller::append(uchar arg)
{
    if (!skipSignature)
        qIterAppend(&iterator, ba, DBUS_TYPE_BYTE, &arg);
}

inline void QDBusMarshaller::append(bool arg)
{
    dbus_bool_t cast = arg;
    if (!skipSignature)
        qIterAppend(&iterator, ba, DBUS_TYPE_BOOLEAN, &cast);
}

inline void QDBusMarshaller::append(short arg)
{
    if (!skipSignature)
        qIterAppend(&iterator, ba, DBUS_TYPE_INT16, &arg);
}

inline void QDBusMarshaller::append(ushort arg)
{
    if (!skipSignature)
        qIterAppend(&iterator, ba, DBUS_TYPE_UINT16, &arg);
}

inline void QDBusMarshaller::append(int arg)
{
    if (!skipSignature)
        qIterAppend(&iterator, ba, DBUS_TYPE_INT32, &arg);
}

inline void QDBusMarshaller::append(uint arg)
{
    if (!skipSignature)
        qIterAppend(&iterator, ba, DBUS_TYPE_UINT32, &arg);
}

inline void QDBusMarshaller::append(qlonglong arg)
{
    if (!skipSignature)
        qIterAppend(&iterator, ba, DBUS_TYPE_INT64, &arg);
}

inline void QDBusMarshaller::append(qulonglong arg)
{
    if (!skipSignature)
        qIterAppend(&iterator, ba, DBUS_TYPE_UINT64, &arg);
}

inline void QDBusMarshaller::append(double arg)
{
    if (!skipSignature)
        qIterAppend(&iterator, ba, DBUS_TYPE_DOUBLE, &arg);
}

void QDBusMarshaller::append(const QString &arg)
{
    QByteArray data = arg.toUtf8();
    const char *cdata = data.constData();
    if (!skipSignature)
        qIterAppend(&iterator, ba, DBUS_TYPE_STRING, &cdata);
}

inline void QDBusMarshaller::append(const QDBusObjectPath &arg)
{
    QByteArray data = arg.path().toUtf8();
    if (!ba && data.isEmpty()) {
        error("Invalid object path passed in arguments"_L1);
    } else {
        const char *cdata = data.constData();
        if (!skipSignature)
            qIterAppend(&iterator, ba, DBUS_TYPE_OBJECT_PATH, &cdata);
    }
}

inline void QDBusMarshaller::append(const QDBusSignature &arg)
{
    QByteArray data = arg.signature().toUtf8();
    if (!ba && data.isNull()) {
        error("Invalid signature passed in arguments"_L1);
    } else {
        const char *cdata = data.constData();
        if (!skipSignature)
            qIterAppend(&iterator, ba, DBUS_TYPE_SIGNATURE, &cdata);
    }
}

inline void QDBusMarshaller::append(const QDBusUnixFileDescriptor &arg)
{
    int fd = arg.fileDescriptor();
    if (!ba && fd == -1) {
        error("Invalid file descriptor passed in arguments"_L1);
    } else {
        if (!skipSignature)
            qIterAppend(&iterator, ba, DBUS_TYPE_UNIX_FD, &fd);
    }
}

inline void QDBusMarshaller::append(const QByteArray &arg)
{
    if (ba) {
        if (!skipSignature)
            *ba += DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_BYTE_AS_STRING;
        return;
    }

    const char* cdata = arg.constData();
    DBusMessageIter subiterator;
    q_dbus_message_iter_open_container(&iterator, DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE_AS_STRING,
                                     &subiterator);
    q_dbus_message_iter_append_fixed_array(&subiterator, DBUS_TYPE_BYTE, &cdata, arg.size());
    q_dbus_message_iter_close_container(&iterator, &subiterator);
}

inline bool QDBusMarshaller::append(const QDBusVariant &arg)
{
    if (ba) {
        if (!skipSignature)
            *ba += DBUS_TYPE_VARIANT_AS_STRING;
        return true;
    }

    const QVariant &value = arg.variant();
    QMetaType id = value.metaType();
    if (!id.isValid()) {
        qWarning("QDBusMarshaller: cannot add a null QDBusVariant");
        error("Invalid QVariant passed in arguments"_L1);
        return false;
    }

    QByteArray tmpSignature;
    const char *signature = nullptr;
    if (id == QDBusMetaTypeId::argument()) {
        // take the signature from the QDBusArgument object we're marshalling
        tmpSignature =
            qvariant_cast<QDBusArgument>(value).currentSignature().toLatin1();
        signature = tmpSignature.constData();
    } else {
        // take the signatuer from the metatype we're marshalling
        signature = QDBusMetaType::typeToSignature(id);
    }
    if (!signature) {
        unregisteredTypeError(id);
        return false;
    }

    QDBusMarshaller sub(capabilities);
    open(sub, DBUS_TYPE_VARIANT, signature);
    bool isOk = sub.appendVariantInternal(value);
    // don't call sub.close(): it auto-closes

    return isOk;
}

inline void QDBusMarshaller::append(const QStringList &arg)
{
    if (ba) {
        if (!skipSignature)
            *ba += DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_STRING_AS_STRING;
        return;
    }

    QDBusMarshaller sub(capabilities);
    open(sub, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING_AS_STRING);
    for (const QString &s : arg)
        sub.append(s);
    // don't call sub.close(): it auto-closes
}

inline QDBusMarshaller *QDBusMarshaller::beginStructure()
{
    return beginCommon(DBUS_TYPE_STRUCT, nullptr);
}

inline QDBusMarshaller *QDBusMarshaller::beginArray(QMetaType id)
{
    const char *signature = QDBusMetaType::typeToSignature(id);
    if (!signature) {
        unregisteredTypeError(id);
        return this;
    }

    return beginCommon(DBUS_TYPE_ARRAY, signature);
}

inline QDBusMarshaller *QDBusMarshaller::beginMap(QMetaType kid, QMetaType vid)
{
    const char *ksignature = QDBusMetaType::typeToSignature(kid);
    if (!ksignature) {
        unregisteredTypeError(kid);
        return this;
    }
    if (ksignature[1] != 0 || !QDBusUtil::isValidBasicType(*ksignature)) {
QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wformat-overflow")
        qWarning("QDBusMarshaller: type '%s' (%d) cannot be used as the key type in a D-Bus map.",
                 kid.name(), kid.id());
QT_WARNING_POP
        error("Type %1 passed in arguments cannot be used as a key in a map"_L1
              .arg(QLatin1StringView(kid.name())));
        return this;
    }

    const char *vsignature = QDBusMetaType::typeToSignature(vid);
    if (!vsignature) {
        unregisteredTypeError(vid);
        return this;
    }

    QByteArray signature;
    signature = DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING;
    signature += ksignature;
    signature += vsignature;
    signature += DBUS_DICT_ENTRY_END_CHAR_AS_STRING;
    return beginCommon(DBUS_TYPE_ARRAY, signature);
}

inline QDBusMarshaller *QDBusMarshaller::beginMapEntry()
{
    return beginCommon(DBUS_TYPE_DICT_ENTRY, nullptr);
}

void QDBusMarshaller::open(QDBusMarshaller &sub, int code, const char *signature)
{
    sub.parent = this;
    sub.ba = ba;
    sub.ok = true;
    sub.capabilities = capabilities;
    sub.skipSignature = skipSignature;

    if (ba) {
        if (!skipSignature) {
            switch (code) {
            case DBUS_TYPE_ARRAY:
                *ba += char(code);
                *ba += signature;
                Q_FALLTHROUGH();

            case DBUS_TYPE_DICT_ENTRY:
                sub.closeCode = 0;
                sub.skipSignature = true;
                break;

            case DBUS_TYPE_STRUCT:
                *ba += DBUS_STRUCT_BEGIN_CHAR;
                sub.closeCode = DBUS_STRUCT_END_CHAR;
                break;
            }
        }
    } else {
        q_dbus_message_iter_open_container(&iterator, code, signature, &sub.iterator);
    }
}

QDBusMarshaller *QDBusMarshaller::beginCommon(int code, const char *signature)
{
    QDBusMarshaller *d = new QDBusMarshaller(capabilities);
    open(*d, code, signature);
    return d;
}

inline QDBusMarshaller *QDBusMarshaller::endStructure()
{ return endCommon(); }

inline QDBusMarshaller *QDBusMarshaller::endArray()
{ return endCommon(); }

inline QDBusMarshaller *QDBusMarshaller::endMap()
{ return endCommon(); }

inline QDBusMarshaller *QDBusMarshaller::endMapEntry()
{ return endCommon(); }

QDBusMarshaller *QDBusMarshaller::endCommon()
{
    QDBusMarshaller *retval = parent;
    delete this;
    return retval;
}

void QDBusMarshaller::close()
{
    if (ba) {
        if (!skipSignature && closeCode)
            *ba += closeCode;
    } else if (parent) {
        q_dbus_message_iter_close_container(&parent->iterator, &iterator);
    }
}

void QDBusMarshaller::error(const QString &msg)
{
    ok = false;
    if (parent)
        parent->error(msg);
    else
        errorString = msg;
}

bool QDBusMarshaller::appendVariantInternal(const QVariant &arg)
{
    QMetaType id = arg.metaType();
    if (!id.isValid()) {
        qWarning("QDBusMarshaller: cannot add an invalid QVariant");
        error("Invalid QVariant passed in arguments"_L1);
        return false;
    }

    // intercept QDBusArgument parameters here
    if (id == QDBusMetaTypeId::argument()) {
        QDBusArgument dbusargument = qvariant_cast<QDBusArgument>(arg);
        QDBusArgumentPrivate *d = QDBusArgumentPrivate::d(dbusargument);
        if (!d->message)
            return false;       // can't append this one...

        QDBusDemarshaller demarshaller(capabilities);
        demarshaller.message = q_dbus_message_ref(d->message);

        if (d->direction == Direction::Demarshalling) {
            // it's demarshalling; just copy
            demarshaller.iterator = static_cast<QDBusDemarshaller *>(d)->iterator;
        } else {
            // it's marshalling; start over
            if (!q_dbus_message_iter_init(demarshaller.message, &demarshaller.iterator))
                return false;   // error!
        }

        return appendCrossMarshalling(&demarshaller);
    }

    const char *signature = QDBusMetaType::typeToSignature(id);
    if (!signature) {
        unregisteredTypeError(id);
        return false;
    }

    switch (*signature) {
#ifdef __OPTIMIZE__
    case DBUS_TYPE_BYTE:
    case DBUS_TYPE_INT16:
    case DBUS_TYPE_UINT16:
    case DBUS_TYPE_INT32:
    case DBUS_TYPE_UINT32:
    case DBUS_TYPE_INT64:
    case DBUS_TYPE_UINT64:
    case DBUS_TYPE_DOUBLE:
        qIterAppend(&iterator, ba, *signature, arg.constData());
        return true;
    case DBUS_TYPE_BOOLEAN:
        append( arg.toBool() );
        return true;
#else
    case DBUS_TYPE_BYTE:
        append( qvariant_cast<uchar>(arg) );
        return true;
    case DBUS_TYPE_BOOLEAN:
        append( arg.toBool() );
        return true;
    case DBUS_TYPE_INT16:
        append( qvariant_cast<short>(arg) );
        return true;
    case DBUS_TYPE_UINT16:
        append( qvariant_cast<ushort>(arg) );
        return true;
    case DBUS_TYPE_INT32:
        append( static_cast<dbus_int32_t>(arg.toInt()) );
        return true;
    case DBUS_TYPE_UINT32:
        append( static_cast<dbus_uint32_t>(arg.toUInt()) );
        return true;
    case DBUS_TYPE_INT64:
        append( arg.toLongLong() );
        return true;
    case DBUS_TYPE_UINT64:
        append( arg.toULongLong() );
        return true;
    case DBUS_TYPE_DOUBLE:
        append( arg.toDouble() );
        return true;
#endif

    case DBUS_TYPE_STRING:
        append( arg.toString() );
        return true;
    case DBUS_TYPE_OBJECT_PATH:
        append( qvariant_cast<QDBusObjectPath>(arg) );
        return true;
    case DBUS_TYPE_SIGNATURE:
        append( qvariant_cast<QDBusSignature>(arg) );
        return true;

    // compound types:
    case DBUS_TYPE_VARIANT:
        // nested QVariant
        return append( qvariant_cast<QDBusVariant>(arg) );

    case DBUS_TYPE_ARRAY:
        // could be many things
        // find out what kind of array it is
        switch (arg.metaType().id()) {
        case QMetaType::QStringList:
            append( arg.toStringList() );
            return true;

        case QMetaType::QByteArray:
            append( arg.toByteArray() );
            return true;

        default:
            ;
        }
        Q_FALLTHROUGH();

    case DBUS_TYPE_STRUCT:
    case DBUS_STRUCT_BEGIN_CHAR:
        return appendRegisteredType( arg );

    case DBUS_TYPE_DICT_ENTRY:
    case DBUS_DICT_ENTRY_BEGIN_CHAR:
        qFatal("QDBusMarshaller::appendVariantInternal got a DICT_ENTRY!");
        return false;

    case DBUS_TYPE_UNIX_FD:
        if (capabilities & QDBusConnection::UnixFileDescriptorPassing || ba) {
            append(qvariant_cast<QDBusUnixFileDescriptor>(arg));
            return true;
        }
        Q_FALLTHROUGH();

    default:
        qWarning("QDBusMarshaller::appendVariantInternal: Found unknown D-Bus type '%s'",
                 signature);
        return false;
    }

    return true;
}

bool QDBusMarshaller::appendRegisteredType(const QVariant &arg)
{
    ref.ref();                  // reference up
    QDBusArgument self(QDBusArgumentPrivate::create(this));
    return QDBusMetaType::marshall(self, arg.metaType(), arg.constData());
}

bool QDBusMarshaller::appendCrossMarshalling(QDBusDemarshaller *demarshaller)
{
    int code = q_dbus_message_iter_get_arg_type(&demarshaller->iterator);
    if (QDBusUtil::isValidBasicType(code)) {
        // easy: just append
        // do exactly like the D-Bus docs suggest
        // (see apidocs for q_dbus_message_iter_get_basic)

        qlonglong value;
        q_dbus_message_iter_get_basic(&demarshaller->iterator, &value);
        q_dbus_message_iter_next(&demarshaller->iterator);
        q_dbus_message_iter_append_basic(&iterator, code, &value);
        return true;
    }

    if (code == DBUS_TYPE_ARRAY) {
        int element = q_dbus_message_iter_get_element_type(&demarshaller->iterator);
        if (QDBusUtil::isValidFixedType(element) && element != DBUS_TYPE_UNIX_FD) {
            // another optimization: fixed size arrays
            // code is exactly like QDBusDemarshaller::toByteArray
            DBusMessageIter sub;
            q_dbus_message_iter_recurse(&demarshaller->iterator, &sub);
            q_dbus_message_iter_next(&demarshaller->iterator);
            int len;
            void* data;
            q_dbus_message_iter_get_fixed_array(&sub,&data,&len);

            char signature[2] = { char(element), 0 };
            q_dbus_message_iter_open_container(&iterator, DBUS_TYPE_ARRAY, signature, &sub);
            q_dbus_message_iter_append_fixed_array(&sub, element, &data, len);
            q_dbus_message_iter_close_container(&iterator, &sub);

            return true;
        }
    }

    // We have to recurse
    QDBusDemarshaller *drecursed = demarshaller->beginCommon();

    QDBusMarshaller mrecursed(capabilities);  // create on the stack makes it autoclose
    QByteArray subSignature;
    const char *sig = nullptr;
    if (code == DBUS_TYPE_VARIANT || code == DBUS_TYPE_ARRAY) {
        subSignature = drecursed->currentSignature().toLatin1();
        if (!subSignature.isEmpty())
            sig = subSignature.constData();
    }
    open(mrecursed, code, sig);

    while (!drecursed->atEnd()) {
        if (!mrecursed.appendCrossMarshalling(drecursed)) {
            delete drecursed;
            return false;
        }
    }

    delete drecursed;
    return true;
}

QT_END_NAMESPACE

#endif // QT_NO_DBUS
