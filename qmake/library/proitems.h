/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the qmake application of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef PROITEMS_H
#define PROITEMS_H

#include "qmake_global.h"

#include <QString>
#include <QStringList>
#include <QHash>
#include <QTextStream>

QT_BEGIN_NAMESPACE

#if 0
#  define PROITEM_EXPLICIT explicit
#else
#  define PROITEM_EXPLICIT
#endif

class ProKey;
class ProStringList;

class ProString {
public:
    ProString() {}
    ProString(const ProString &other) : m_string(other.m_string) {}
    PROITEM_EXPLICIT ProString(const QString &str) : m_string(str) {}
    PROITEM_EXPLICIT ProString(const char *str) : m_string(QLatin1String(str)) {}
    void clear() { m_string.clear(); }

    ProString &prepend(const ProString &other) { m_string.prepend(other.m_string); return *this; }
    ProString &append(const ProString &other) { m_string.append(other.m_string); return *this; }
    ProString &append(const QString &other) { m_string.append(other); return *this; }
    ProString &append(const char *other) { m_string.append(QLatin1String(other)); return *this; }
    ProString &append(QChar other) { m_string.append(other); return *this; }
    ProString &operator+=(const ProString &other) { return append(other); }
    ProString &operator+=(const QString &other) { return append(other); }
    ProString &operator+=(const char *other) { return append(other); }
    ProString &operator+=(QChar other) { return append(other); }

    void chop(int n) { m_string.chop(n); }
    void chopFront(int n) { m_string.remove(0, n); }

    bool operator==(const ProString &other) const { return toQStringRef() == other.toQStringRef(); }
    bool operator==(const QString &other) const { return toQStringRef() == other; }
    bool operator==(QLatin1String other) const  { return toQStringRef() == other; }
    bool operator==(const char *other) const { return toQStringRef() == QLatin1String(other); }
    bool operator!=(const ProString &other) const { return !(*this == other); }
    bool operator!=(const QString &other) const { return !(*this == other); }
    bool operator!=(QLatin1String other) const { return !(*this == other); }
    bool operator!=(const char *other) const { return !(*this == other); }
    bool isNull() const { return m_string.isNull(); }
    bool isEmpty() const { return m_string.isEmpty(); }
    int length() const { return m_string.size(); }
    int size() const { return m_string.size(); }
    QChar at(int i) const { return m_string.at(i); }
    const QChar *constData() const { return m_string.constData(); }
    ProString mid(int off, int len = -1) const { return m_string.mid(off, len); }
    ProString left(int len) const { return mid(0, len); }
    ProString right(int len) const { return mid(qMax(0, size() - len)); }
    ProString trimmed() const { return m_string.trimmed(); }
    int compare(const ProString &sub, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toQStringRef().compare(sub.toQStringRef(), cs); }
    int compare(const QString &sub, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toQStringRef().compare(sub, cs); }
    int compare(const char *sub, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toQStringRef().compare(QLatin1String(sub), cs); }
    bool startsWith(const ProString &sub) const { return toQStringRef().startsWith(sub.toQStringRef()); }
    bool startsWith(const QString &sub) const { return toQStringRef().startsWith(sub); }
    bool startsWith(const char *sub) const { return toQStringRef().startsWith(QLatin1String(sub)); }
    bool startsWith(QChar c) const { return toQStringRef().startsWith(c); }
    bool endsWith(const ProString &sub) const { return toQStringRef().endsWith(sub.toQStringRef()); }
    bool endsWith(const QString &sub) const { return toQStringRef().endsWith(sub); }
    bool endsWith(const char *sub) const { return toQStringRef().endsWith(QLatin1String(sub)); }
    bool endsWith(QChar c) const { return toQStringRef().endsWith(c); }
    int indexOf(const QString &s, int from = 0, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toQStringRef().indexOf(s, from, cs); }
    int indexOf(const char *s, int from = 0, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toQStringRef().indexOf(QLatin1String(s), from, cs); }
    int indexOf(QChar c, int from = 0, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toQStringRef().indexOf(c, from, cs); }
    int lastIndexOf(const QString &s, int from = -1, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toQStringRef().lastIndexOf(s, from, cs); }
    int lastIndexOf(const char *s, int from = -1, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toQStringRef().lastIndexOf(QLatin1String(s), from, cs); }
    int lastIndexOf(QChar c, int from = -1, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toQStringRef().lastIndexOf(c, from, cs); }
    bool contains(const QString &s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return indexOf(s, 0, cs) >= 0; }
    bool contains(const char *s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return indexOf(QLatin1String(s), 0, cs) >= 0; }
    bool contains(QChar c, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return indexOf(c, 0, cs) >= 0; }
    int toInt(bool *ok = 0) const { return toQString().toInt(ok); } // XXX optimize
    short toShort(bool *ok = 0) const { return toQString().toShort(ok); } // XXX optimize

    ALWAYS_INLINE QStringRef toQStringRef() const { return QStringRef(&m_string, 0, m_string.length()); }

    ALWAYS_INLINE ProKey &toKey() { return *(ProKey *)this; }
    ALWAYS_INLINE const ProKey &toKey() const { return *(const ProKey *)this; }

    QString toQString() const { return m_string; }

    QByteArray toLatin1() const { return toQStringRef().toLatin1(); }

private:
    ProString(const ProKey &other);
    ProString &operator=(const ProKey &other);

    QString m_string;
    friend uint qHash(const ProKey &str, uint seed);
    friend QString operator+(const ProString &one, const ProString &two);
    friend QString &operator+=(QString &that, const ProString &other);
    friend class ProKey;
};
Q_DECLARE_TYPEINFO(ProString, Q_MOVABLE_TYPE);

class ProKey : public ProString {
public:
    ALWAYS_INLINE ProKey() : ProString() {}
    explicit ProKey(const QString &str) : ProString(str) {}
    PROITEM_EXPLICIT ProKey(const char *str) : ProString(str) {}

#ifdef Q_CC_MSVC
    // Workaround strange MSVC behaviour when exporting classes with ProKey members.
    ALWAYS_INLINE ProKey(const ProKey &other) : ProString(other.toString()) {}
    ALWAYS_INLINE ProKey &operator=(const ProKey &other)
    {
        toString() = other.toString();
        return *this;
    }
#endif

    ALWAYS_INLINE ProString &toString() { return *(ProString *)this; }
    ALWAYS_INLINE const ProString &toString() const { return *(const ProString *)this; }

private:
    ProKey(const ProString &other);
};
Q_DECLARE_TYPEINFO(ProKey, Q_MOVABLE_TYPE);

inline uint qHash(const ProKey &key, uint seed = 0)
    { return qHash(key.m_string, seed); }
inline QString operator+(const ProString &one, const ProString &two)
    { return one.m_string + two.m_string; }
inline QString operator+(const ProString &one, const QString &two)
    { return one + ProString(two); }
inline QString operator+(const QString &one, const ProString &two)
    { return ProString(one) + two; }

inline QString operator+(const ProString &one, const char *two)
    { return one + ProString(two); } // XXX optimize
inline QString operator+(const char *one, const ProString &two)
    { return ProString(one) + two; } // XXX optimize

inline QString &operator+=(QString &that, const ProString &other)
    { return that += other.toQStringRef(); }

inline bool operator==(const QString &that, const ProString &other)
    { return other == that; }
inline bool operator!=(const QString &that, const ProString &other)
    { return !(other == that); }

inline QTextStream &operator<<(QTextStream &t, const ProString &str)
    { t << str.toQString(); return t; }

class ProStringList : public QList<ProString> {
public:
    ProStringList() {}
    ProStringList(const ProString &str) { *this << str; }
    explicit ProStringList(const QStringList &list) : QList<ProString>(*(const ProStringList *)&list) {}
    QStringList toQStringList() const { return *(const QStringList *)this; }

    ProStringList &operator<<(const ProString &str)
        { QList<ProString>::operator<<(str); return *this; }

    QString join(const QString &sep) const { return toQStringList().join(sep); }

    void remove(int idx) { removeAt(idx); }

    bool contains(const ProString &str, Qt::CaseSensitivity cs = Qt::CaseSensitive) const
        { return contains(str.toQString(), cs); }
    bool contains(const QString &str, Qt::CaseSensitivity cs = Qt::CaseSensitive) const
        { return (*(const QStringList *)this).contains(str, cs); }
    bool contains(const char *str, Qt::CaseSensitivity cs = Qt::CaseSensitive) const
        { return (*(const QStringList *)this).contains(str, cs); }
};
Q_DECLARE_TYPEINFO(ProStringList, Q_MOVABLE_TYPE);

inline ProStringList operator+(const ProStringList &one, const ProStringList &two)
    { ProStringList ret = one; ret += two; return ret; }

typedef QHash<ProKey, ProStringList> ProValueMap;

QT_END_NAMESPACE

#endif // PROITEMS_H
