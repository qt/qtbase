// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef Q_CTF_H
#define Q_CTF_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//
//

#include <qtcoreexports.h>
#include <qobject.h>

QT_BEGIN_NAMESPACE

struct QCtfTraceMetadata;
struct Q_CORE_EXPORT QCtfTracePointProvider
{
    const QString provider;
    QCtfTraceMetadata *metadata;
    QCtfTracePointProvider(const QString &provider)
        : provider(provider), metadata(nullptr)
    {

    }
};

struct Q_CORE_EXPORT QCtfTraceMetadata
{
    const QString name;
    const QString metadata;
    QCtfTraceMetadata(QCtfTracePointProvider &provider, const QString &name, const QString &metadata)
        : name(name), metadata(metadata)
    {
        next = provider.metadata;
        provider.metadata = this;
    }
    QCtfTraceMetadata *next = nullptr;
};

struct QCtfTracePointPrivate;
struct Q_CORE_EXPORT QCtfTracePointEvent
{
    const QCtfTracePointProvider &provider;
    const QString eventName;
    const QString metadata;
    const int size;
    const bool variableSize;

    QCtfTracePointEvent(const QCtfTracePointProvider &provider, const QString &name, const QString &metadata, int size, bool variableSize)
        : provider(provider), eventName(name), metadata(metadata), size(size), variableSize(variableSize)
    {
    }
    QCtfTracePointPrivate *d = nullptr;
};



Q_CORE_EXPORT bool _tracepoint_enabled(const QCtfTracePointEvent &point);
Q_CORE_EXPORT void _do_tracepoint(const QCtfTracePointEvent &point, const QByteArray &arr);
Q_CORE_EXPORT QCtfTracePointPrivate *_initialize_tracepoint(const QCtfTracePointEvent &point);

#ifndef BUILD_LIBRARY
#include <QtCore/qbytearray.h>
#include <QtCore/qstring.h>
#include <QtCore/qurl.h>
namespace trace {
inline void toByteArray(QByteArray &)
{
}

inline void toByteArray(QByteArray &arr, const QString &value)
{
    arr.append(value.toUtf8());
    arr.append((char)0);
}

inline void toByteArray(QByteArray &arr, const QUrl &value)
{
    arr.append(value.toString().toUtf8());
    arr.append((char)0);
}

inline void toByteArray(QByteArray &arr, const QByteArray &data)
{
    arr.append(data);
}

template <typename T>
inline void toByteArray(QByteArray &arr, T value)
{
    arr.append((char *)&value, sizeof(value));
}

template <typename T, typename... Ts>
inline void toByteArray(QByteArray &arr, T value, Ts... args)
{
    toByteArray(arr, value);
    toByteArray(arr, args...);
}

inline QByteArray toByteArray()
{
    return {};
}

template <typename... Ts>
inline QByteArray toByteArray(Ts... args)
{
    QByteArray data;
    toByteArray(data, args...);
    return data;
}

template <typename T>
inline QByteArray toByteArrayFromArray(const T *values, int arraySize)
{
    QByteArray data;
    data.append((char *)values, arraySize * sizeof(T));
    return data;
}

template <typename IntegerType, typename T>
inline QByteArray toByteArrayFromEnum(T value)
{
    IntegerType e = static_cast<IntegerType>(value);
    QByteArray data;
    data.append((char *)&e, sizeof(e));
    return data;
}

inline QByteArray toByteArrayFromCString(const char *str)
{
    QByteArray data;
    if (str && *str != 0)
        data.append(str);
    data.append((char)0);
    return data;
}

static inline void appendFlags(QByteArray &data, quint8 &count, quint32 value)
{
    count = 0;
    quint8 d = 1;
    while (value) {
        if (value&1) {
            data.append(d);
            count++;
        }
        d++;
        value >>= 1;
    }
}

template <typename T>
inline QByteArray toByteArrayFromFlags(QFlags<T> value)
{
    quint32 intValue = static_cast<quint32>(value.toInt());
    quint8 count;
    QByteArray data;
    data.append((char)0);
    if (intValue == 0) {
        data.append((char)0);
        data.data()[0] = 1;
    } else {
        appendFlags(data, count, intValue);
        data.data()[0] = count;
    }
    return data;
}

} // trace

#define _DEFINE_EVENT(provider, event, metadata, size, varSize) \
    static QCtfTracePointEvent _ctf_ ## event = QCtfTracePointEvent(_ctf_provider_ ## provider, QStringLiteral(QT_STRINGIFY(event)), metadata, size, varSize);
#define _DEFINE_METADATA(provider, name, metadata) \
    static QCtfTraceMetadata _ctf_metadata_ ## name = QCtfTraceMetadata(_ctf_provider_ ## provider, QStringLiteral(QT_STRINGIFY(name)), metadata);
#define _DEFINE_TRACEPOINT_PROVIDER(provider) \
    static QCtfTracePointProvider _ctf_provider_ ## provider = QCtfTracePointProvider(QStringLiteral(QT_STRINGIFY(provider)));

#define TRACEPOINT_EVENT(provider, event, metadata, size, varSize) \
    _DEFINE_EVENT(provider, event, metadata, size, varSize)

#define TRACEPOINT_PROVIDER(provider) \
    _DEFINE_TRACEPOINT_PROVIDER(provider)

#define TRACEPOINT_METADATA(provider, name, metadata) \
    _DEFINE_METADATA(provider, name, metadata)

#define tracepoint_enabled(provider, event) \
    _tracepoint_enabled(_ctf_ ## event)

#define do_tracepoint(provider, event, ...)         \
{                                                   \
    auto &tp = _ctf_ ## event;                      \
    if (!tp.d)                                      \
        tp.d = _initialize_tracepoint(tp);          \
    if (tp.d) {                                     \
        QByteArray data(tp.size, 0);                \
        if (!tp.metadata.isEmpty())                 \
            data = trace::toByteArray(__VA_ARGS__); \
        _do_tracepoint(tp, data);                   \
    }                                               \
}

#define tracepoint(provider, name, ...)                 \
    do {                                                \
        if (tracepoint_enabled(provider, name))         \
            do_tracepoint(provider, name, __VA_ARGS__); \
    } while (0)

#endif

class Q_CORE_EXPORT QCtfLib : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(QCtfLib)
public:
    explicit QCtfLib(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~QCtfLib() = default;
    virtual bool tracepointEnabled(const QCtfTracePointEvent &point) = 0;
    virtual void doTracepoint(const QCtfTracePointEvent &point, const QByteArray &arr) = 0;
    virtual bool sessionEnabled() = 0;
    virtual QCtfTracePointPrivate *initializeTracepoint(const QCtfTracePointEvent &point) = 0;
    virtual void shutdown(bool *shutdown) = 0;
};

QT_END_NAMESPACE

#endif
