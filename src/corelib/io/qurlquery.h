/****************************************************************************
**
** Copyright (C) 2012 Intel Corporation.
** Contact: http://www.qt-project.org/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QURLQUERY_H
#define QURLQUERY_H

#include <QtCore/qpair.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qurl.h>

#if QT_DEPRECATED_SINCE(5,0)
#include <QtCore/qstringlist.h>
#endif

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QUrlQueryPrivate;
class Q_CORE_EXPORT QUrlQuery
{
public:
    QUrlQuery();
    explicit QUrlQuery(const QUrl &url);
    explicit QUrlQuery(const QString &queryString);
    QUrlQuery(const QUrlQuery &other);
    QUrlQuery &operator=(const QUrlQuery &other);
#ifdef Q_COMPILER_RVALUE_REFS
    QUrlQuery &operator=(QUrlQuery &&other)
    { qSwap(d, other.d); return *this; }
#endif
    ~QUrlQuery();

    bool operator==(const QUrlQuery &other) const;
    bool operator!=(const QUrlQuery &other) const
    { return !(*this == other); }

    void swap(QUrlQuery &other) { qSwap(d, other.d); }

    bool isEmpty() const;
    bool isDetached() const;
    void clear();

    QString query(QUrl::ComponentFormattingOptions encoding = QUrl::PrettyDecoded) const;
    void setQuery(const QString &queryString);
    QString toString(QUrl::ComponentFormattingOptions encoding = QUrl::PrettyDecoded) const
    { return query(encoding); }

    void setQueryDelimiters(QChar valueDelimiter, QChar pairDelimiter);
    QChar queryValueDelimiter() const;
    QChar queryPairDelimiter() const;

    void setQueryItems(const QList<QPair<QString, QString> > &query);
    QList<QPair<QString, QString> > queryItems(QUrl::ComponentFormattingOptions encoding = QUrl::PrettyDecoded) const;

    bool hasQueryItem(const QString &key) const;
    void addQueryItem(const QString &key, const QString &value);
    void removeQueryItem(const QString &key);
    QString queryItemValue(const QString &key, QUrl::ComponentFormattingOptions encoding = QUrl::PrettyDecoded) const;
    QStringList allQueryItemValues(const QString &key, QUrl::ComponentFormattingOptions encoding = QUrl::PrettyDecoded) const;
    void removeAllQueryItems(const QString &key);

    static QChar defaultQueryValueDelimiter()
    { return QChar(ushort('=')); }
    static QChar defaultQueryPairDelimiter()
    { return QChar(ushort('&')); }

private:
    friend class QUrl;
    QSharedDataPointer<QUrlQueryPrivate> d;
public:
    typedef QSharedDataPointer<QUrlQueryPrivate> DataPtr;
    inline DataPtr &data_ptr() { return d; }
};

Q_DECLARE_TYPEINFO(QUrlQuery, Q_MOVABLE_TYPE);
Q_DECLARE_SHARED(QUrlQuery)

#if QT_DEPRECATED_SINCE(5,0)
inline void QUrl::setQueryItems(const QList<QPair<QString, QString> > &qry)
{ QUrlQuery q(*this); q.setQueryItems(qry); setQuery(q); }
inline void QUrl::addQueryItem(const QString &key, const QString &value)
{ QUrlQuery q(*this); q.addQueryItem(key, value); setQuery(q); }
inline QList<QPair<QString, QString> > QUrl::queryItems() const
{ return QUrlQuery(*this).queryItems(); }
inline bool QUrl::hasQueryItem(const QString &key) const
{ return QUrlQuery(*this).hasQueryItem(key); }
inline QString QUrl::queryItemValue(const QString &key) const
{ return QUrlQuery(*this).queryItemValue(key); }
inline QStringList QUrl::allQueryItemValues(const QString &key) const
{ return QUrlQuery(*this).allQueryItemValues(key); }
inline void QUrl::removeQueryItem(const QString &key)
{ QUrlQuery q(*this); q.removeQueryItem(key); setQuery(q); }
inline void QUrl::removeAllQueryItems(const QString &key)
{ QUrlQuery q(*this); q.removeAllQueryItems(key); }

inline void QUrl::addEncodedQueryItem(const QByteArray &key, const QByteArray &value)
{ QUrlQuery q(*this); q.addQueryItem(QString::fromUtf8(key), QString::fromUtf8(value)); setQuery(q); }
inline bool QUrl::hasEncodedQueryItem(const QByteArray &key) const
{ return QUrlQuery(*this).hasQueryItem(QString::fromUtf8(key)); }
inline QByteArray QUrl::encodedQueryItemValue(const QByteArray &key) const
{ return QUrlQuery(*this).queryItemValue(QString::fromUtf8(key), QUrl::FullyEncoded).toLatin1(); }
inline void QUrl::removeEncodedQueryItem(const QByteArray &key)
{ QUrlQuery q(*this); q.removeQueryItem(QString::fromUtf8(key)); setQuery(q); }
inline void QUrl::removeAllEncodedQueryItems(const QByteArray &key)
{ QUrlQuery q(*this); q.removeAllQueryItems(QString::fromUtf8(key)); }

inline void QUrl::setEncodedQueryItems(const QList<QPair<QByteArray, QByteArray> > &qry)
{
    QUrlQuery q;
    QList<QPair<QByteArray, QByteArray> >::ConstIterator it = qry.constBegin();
    for ( ; it != qry.constEnd(); ++it)
        q.addQueryItem(QString::fromUtf8(it->first), QString::fromUtf8(it->second));
    setQuery(q);
}
inline QList<QPair<QByteArray, QByteArray> > QUrl::encodedQueryItems() const
{
    QList<QPair<QString, QString> > items = QUrlQuery(*this).queryItems(QUrl::FullyEncoded);
    QList<QPair<QString, QString> >::ConstIterator it = items.constBegin();
    QList<QPair<QByteArray, QByteArray> > result;
    result.reserve(items.size());
    for ( ; it != items.constEnd(); ++it)
        result << qMakePair(it->first.toLatin1(), it->second.toLatin1());
    return result;
}
inline QList<QByteArray> QUrl::allEncodedQueryItemValues(const QByteArray &key) const
{
    QStringList items = QUrlQuery(*this).allQueryItemValues(QString::fromUtf8(key), QUrl::FullyEncoded);
    QList<QByteArray> result;
    result.reserve(items.size());
    Q_FOREACH (const QString &item, items)
        result << item.toLatin1();
    return result;
}
#endif

QT_END_NAMESPACE

QT_END_HEADER

#endif // QURLQUERY_H
