// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDBUSARGUMENT_P_H
#define QDBUSARGUMENT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLibrary class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtDBus/private/qtdbusglobal_p.h>
#include <qdbusargument.h>
#include "qdbusunixfiledescriptor.h"
#include "qdbus_symbols_p.h"

#ifndef QT_NO_DBUS

#ifndef DBUS_TYPE_UNIX_FD
# define DBUS_TYPE_UNIX_FD int('h')
# define DBUS_TYPE_UNIX_FD_AS_STRING "h"
#endif

QT_BEGIN_NAMESPACE

class QDBusMarshaller;
class QDBusDemarshaller;
class QDBusArgumentPrivate
{
public:
    inline QDBusArgumentPrivate(int flags = 0)
        : message(nullptr), ref(1), capabilities(flags)
    { }
    virtual ~QDBusArgumentPrivate();

    static bool checkRead(QDBusArgumentPrivate *d);
    static bool checkReadAndDetach(QDBusArgumentPrivate *&d);
    static bool checkWrite(QDBusArgumentPrivate *&d);

    QDBusMarshaller *marshaller();
    QDBusDemarshaller *demarshaller();

    static QByteArray createSignature(int id);
    static inline QDBusArgument create(QDBusArgumentPrivate *d)
    {
        QDBusArgument q(d);
        return q;
    }
    static inline QDBusArgumentPrivate *d(QDBusArgument &q)
    { return q.d; }

public:
    DBusMessage *message;
    QAtomicInt ref;
    int capabilities;
    enum Direction {
        Marshalling,
        Demarshalling
    } direction;
};

class QDBusMarshaller: public QDBusArgumentPrivate
{
public:
    QDBusMarshaller(int flags) : QDBusArgumentPrivate(flags), parent(nullptr), ba(nullptr), closeCode(0), ok(true), skipSignature(false)
    { direction = Marshalling; }
    ~QDBusMarshaller();

    QString currentSignature();

    void append(uchar arg);
    void append(bool arg);
    void append(short arg);
    void append(ushort arg);
    void append(int arg);
    void append(uint arg);
    void append(qlonglong arg);
    void append(qulonglong arg);
    void append(double arg);
    void append(const QString &arg);
    void append(const QDBusObjectPath &arg);
    void append(const QDBusSignature &arg);
    void append(const QDBusUnixFileDescriptor &arg);
    void append(const QStringList &arg);
    void append(const QByteArray &arg);
    bool append(const QDBusVariant &arg); // this one can fail

    QDBusMarshaller *beginStructure();
    QDBusMarshaller *endStructure();
    QDBusMarshaller *beginArray(QMetaType id);
    QDBusMarshaller *endArray();
    QDBusMarshaller *beginMap(QMetaType kid, QMetaType vid);
    QDBusMarshaller *endMap();
    QDBusMarshaller *beginMapEntry();
    QDBusMarshaller *endMapEntry();
    QDBusMarshaller *beginCommon(int code, const char *signature);
    QDBusMarshaller *endCommon();
    void open(QDBusMarshaller &sub, int code, const char *signature);
    void close();
    void error(const QString &message);

    bool appendVariantInternal(const QVariant &arg);
    bool appendRegisteredType(const QVariant &arg);
    bool appendCrossMarshalling(QDBusDemarshaller *arg);

public:
    DBusMessageIter iterator;
    QDBusMarshaller *parent;
    QByteArray *ba;
    QString errorString;
    char closeCode;
    bool ok;
    bool skipSignature;

private:
    Q_DECL_COLD_FUNCTION void unregisteredTypeError(QMetaType t);
    Q_DISABLE_COPY_MOVE(QDBusMarshaller)
};

class QDBusDemarshaller: public QDBusArgumentPrivate
{
public:
    inline QDBusDemarshaller(int flags) : QDBusArgumentPrivate(flags), parent(nullptr)
    { direction = Demarshalling; }
    ~QDBusDemarshaller();

    QString currentSignature();

    uchar toByte();
    bool toBool();
    ushort toUShort();
    short toShort();
    int toInt();
    uint toUInt();
    qlonglong toLongLong();
    qulonglong toULongLong();
    double toDouble();
    QString toString();
    QDBusObjectPath toObjectPath();
    QDBusSignature toSignature();
    QDBusUnixFileDescriptor toUnixFileDescriptor();
    QDBusVariant toVariant();
    QStringList toStringList();
    QByteArray toByteArray();

    QDBusDemarshaller *beginStructure();
    QDBusDemarshaller *endStructure();
    QDBusDemarshaller *beginArray();
    QDBusDemarshaller *endArray();
    QDBusDemarshaller *beginMap();
    QDBusDemarshaller *endMap();
    QDBusDemarshaller *beginMapEntry();
    QDBusDemarshaller *endMapEntry();
    QDBusDemarshaller *beginCommon();
    QDBusDemarshaller *endCommon();
    QDBusArgument duplicate();
    inline void close() { }

    bool atEnd();

    QVariant toVariantInternal();
    QDBusArgument::ElementType currentType();
    bool isCurrentTypeStringLike();

public:
    DBusMessageIter iterator;
    QDBusDemarshaller *parent;

private:
    Q_DISABLE_COPY_MOVE(QDBusDemarshaller)
    QString toStringUnchecked();
    QDBusObjectPath toObjectPathUnchecked();
    QDBusSignature toSignatureUnchecked();
    QStringList toStringListUnchecked();
    QByteArray toByteArrayUnchecked();
};

inline QDBusMarshaller *QDBusArgumentPrivate::marshaller()
{ return static_cast<QDBusMarshaller *>(this); }

inline QDBusDemarshaller *QDBusArgumentPrivate::demarshaller()
{ return static_cast<QDBusDemarshaller *>(this); }

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif
