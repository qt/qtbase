/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qlist.h>

#ifndef QSTRINGLIST_H
#define QSTRINGLIST_H

#include <QtCore/qalgorithms.h>
#include <QtCore/qcontainertools_impl.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringmatcher.h>

QT_BEGIN_NAMESPACE

class QRegularExpression;

#if !defined(QT_NO_JAVA_STYLE_ITERATORS)
using QStringListIterator = QListIterator<QString>;
using QMutableStringListIterator = QMutableListIterator<QString>;
#endif

class QStringList;

#ifdef Q_QDOC
class QStringList : public QList<QString>
#else
template <> struct QListSpecialMethods<QString>
#endif
{
#ifndef Q_QDOC
protected:
    ~QListSpecialMethods() = default;
#endif
public:
    inline void sort(Qt::CaseSensitivity cs = Qt::CaseSensitive);
    inline qsizetype removeDuplicates();

#if QT_STRINGVIEW_LEVEL < 2
    inline QString join(const QString &sep) const;
#endif
    inline QString join(QStringView sep) const;
    inline QString join(QLatin1String sep) const;
    inline QString join(QChar sep) const;

    inline QStringList filter(QStringView str, Qt::CaseSensitivity cs = Qt::CaseSensitive) const;
    inline QStringList &replaceInStrings(QStringView before, QStringView after, Qt::CaseSensitivity cs = Qt::CaseSensitive);
#if QT_STRINGVIEW_LEVEL < 2
    inline QStringList filter(const QString &str, Qt::CaseSensitivity cs = Qt::CaseSensitive) const;
    inline QStringList &replaceInStrings(const QString &before, const QString &after, Qt::CaseSensitivity cs = Qt::CaseSensitive);
    inline QStringList &replaceInStrings(const QString &before, QStringView after, Qt::CaseSensitivity cs = Qt::CaseSensitive);
    inline QStringList &replaceInStrings(QStringView before, const QString &after, Qt::CaseSensitivity cs = Qt::CaseSensitive);
#endif

#if QT_CONFIG(regularexpression)
    inline QStringList filter(const QRegularExpression &re) const;
    inline QStringList &replaceInStrings(const QRegularExpression &re, const QString &after);
#endif // QT_CONFIG(regularexpression)

#ifndef Q_QDOC
private:
    inline QStringList *self();
    inline const QStringList *self() const;
};

// ### Qt6: check if there's a better way
class QStringList : public QList<QString>
{
#endif
public:
    inline QStringList() noexcept { }
    inline explicit QStringList(const QString &i) { append(i); }
    inline QStringList(const QList<QString> &l) : QList<QString>(l) { }
    inline QStringList(QList<QString> &&l) noexcept : QList<QString>(std::move(l)) { }
    inline QStringList(std::initializer_list<QString> args) : QList<QString>(args) { }
    template <typename InputIterator, QtPrivate::IfIsInputIterator<InputIterator> = true>
    inline QStringList(InputIterator first, InputIterator last)
        : QList<QString>(first, last) { }

    QStringList &operator=(const QList<QString> &other)
    { QList<QString>::operator=(other); return *this; }
    QStringList &operator=(QList<QString> &&other) noexcept
    { QList<QString>::operator=(std::move(other)); return *this; }

#if QT_STRINGVIEW_LEVEL < 2
    inline bool contains(const QString &str, Qt::CaseSensitivity cs = Qt::CaseSensitive) const;
#endif
    inline bool contains(QLatin1String str, Qt::CaseSensitivity cs = Qt::CaseSensitive) const;
    inline bool contains(QStringView str, Qt::CaseSensitivity cs = Qt::CaseSensitive) const;

    inline QStringList operator+(const QStringList &other) const
    { QStringList n = *this; n += other; return n; }
    inline QStringList &operator<<(const QString &str)
    { append(str); return *this; }
    inline QStringList &operator<<(const QStringList &l)
    { *this += l; return *this; }
    inline QStringList &operator<<(const QList<QString> &l)
    { *this += l; return *this; }

    inline qsizetype indexOf(QStringView str, qsizetype from = 0) const;
    inline qsizetype indexOf(QLatin1String str, qsizetype from = 0) const;

    inline qsizetype lastIndexOf(QStringView str, qsizetype from = -1) const;
    inline qsizetype lastIndexOf(QLatin1String str, qsizetype from = -1) const;

#if QT_CONFIG(regularexpression)
    inline qsizetype indexOf(const QRegularExpression &re, qsizetype from = 0) const;
    inline qsizetype lastIndexOf(const QRegularExpression &re, qsizetype from = -1) const;
#endif // QT_CONFIG(regularexpression)

    using QList<QString>::indexOf;
    using QList<QString>::lastIndexOf;
};

Q_DECLARE_TYPEINFO(QStringList, Q_MOVABLE_TYPE);

#ifndef Q_QDOC
inline QStringList *QListSpecialMethods<QString>::self()
{ return static_cast<QStringList *>(this); }
inline const QStringList *QListSpecialMethods<QString>::self() const
{ return static_cast<const QStringList *>(this); }

namespace QtPrivate {
    void Q_CORE_EXPORT QStringList_sort(QStringList *that, Qt::CaseSensitivity cs);
    qsizetype Q_CORE_EXPORT QStringList_removeDuplicates(QStringList *that);
    QString Q_CORE_EXPORT QStringList_join(const QStringList *that, QStringView sep);
    QString Q_CORE_EXPORT QStringList_join(const QStringList *that, const QChar *sep, qsizetype seplen);
    Q_CORE_EXPORT QString QStringList_join(const QStringList &list, QLatin1String sep);
    QStringList Q_CORE_EXPORT QStringList_filter(const QStringList *that, QStringView str,
                                               Qt::CaseSensitivity cs);
    bool Q_CORE_EXPORT QStringList_contains(const QStringList *that, QStringView str, Qt::CaseSensitivity cs);
    bool Q_CORE_EXPORT QStringList_contains(const QStringList *that, QLatin1String str, Qt::CaseSensitivity cs);
    void Q_CORE_EXPORT QStringList_replaceInStrings(QStringList *that, QStringView before, QStringView after,
                                      Qt::CaseSensitivity cs);

#if QT_CONFIG(regularexpression)
    void Q_CORE_EXPORT QStringList_replaceInStrings(QStringList *that, const QRegularExpression &rx, const QString &after);
    QStringList Q_CORE_EXPORT QStringList_filter(const QStringList *that, const QRegularExpression &re);
    qsizetype Q_CORE_EXPORT QStringList_indexOf(const QStringList *that, const QRegularExpression &re, qsizetype from);
    qsizetype Q_CORE_EXPORT QStringList_lastIndexOf(const QStringList *that, const QRegularExpression &re, qsizetype from);
#endif // QT_CONFIG(regularexpression)
}

inline void QListSpecialMethods<QString>::sort(Qt::CaseSensitivity cs)
{
    QtPrivate::QStringList_sort(self(), cs);
}

inline qsizetype QListSpecialMethods<QString>::removeDuplicates()
{
    return QtPrivate::QStringList_removeDuplicates(self());
}

#if QT_STRINGVIEW_LEVEL < 2
inline QString QListSpecialMethods<QString>::join(const QString &sep) const
{
    return QtPrivate::QStringList_join(self(), sep.constData(), sep.length());
}
#endif

inline QString QListSpecialMethods<QString>::join(QStringView sep) const
{
    return QtPrivate::QStringList_join(self(), sep);
}

QString QListSpecialMethods<QString>::join(QLatin1String sep) const
{
    return QtPrivate::QStringList_join(*self(), sep);
}

inline QString QListSpecialMethods<QString>::join(QChar sep) const
{
    return QtPrivate::QStringList_join(self(), &sep, 1);
}

inline QStringList QListSpecialMethods<QString>::filter(QStringView str, Qt::CaseSensitivity cs) const
{
    return QtPrivate::QStringList_filter(self(), str, cs);
}

#if QT_STRINGVIEW_LEVEL < 2
inline QStringList QListSpecialMethods<QString>::filter(const QString &str, Qt::CaseSensitivity cs) const
{
    return QtPrivate::QStringList_filter(self(), str, cs);
}
#endif

#if QT_STRINGVIEW_LEVEL < 2
inline bool QStringList::contains(const QString &str, Qt::CaseSensitivity cs) const
{
    return QtPrivate::QStringList_contains(this, str, cs);
}
#endif

inline bool QStringList::contains(QLatin1String str, Qt::CaseSensitivity cs) const
{
    return QtPrivate::QStringList_contains(this, str, cs);
}

inline bool QStringList::contains(QStringView str, Qt::CaseSensitivity cs) const
{
    return QtPrivate::QStringList_contains(this, str, cs);
}

inline QStringList &QListSpecialMethods<QString>::replaceInStrings(QStringView before, QStringView after, Qt::CaseSensitivity cs)
{
    QtPrivate::QStringList_replaceInStrings(self(), before, after, cs);
    return *self();
}

#if QT_STRINGVIEW_LEVEL < 2
inline QStringList &QListSpecialMethods<QString>::replaceInStrings(const QString &before, const QString &after, Qt::CaseSensitivity cs)
{
    QtPrivate::QStringList_replaceInStrings(self(), before, after, cs);
    return *self();
}

inline QStringList &QListSpecialMethods<QString>::replaceInStrings(QStringView before, const QString &after, Qt::CaseSensitivity cs)
{
    QtPrivate::QStringList_replaceInStrings(self(), before, qToStringViewIgnoringNull(after), cs);
    return *self();
}

inline QStringList &QListSpecialMethods<QString>::replaceInStrings(const QString &before, QStringView after, Qt::CaseSensitivity cs)
{
    QtPrivate::QStringList_replaceInStrings(self(), QStringView(before), after, cs);
    return *self();
}
#endif

inline QStringList operator+(const QList<QString> &one, const QStringList &other)
{
    QStringList n = one;
    n += other;
    return n;
}

inline qsizetype QStringList::indexOf(QStringView string, qsizetype from) const
{
    return QtPrivate::indexOf<QString, QStringView>(*this, string, from);
}

inline qsizetype QStringList::indexOf(QLatin1String string, qsizetype from) const
{
    return QtPrivate::indexOf<QString, QLatin1String>(*this, string, from);
}

inline qsizetype QStringList::lastIndexOf(QStringView string, qsizetype from) const
{
    return QtPrivate::lastIndexOf<QString, QStringView>(*this, string, from);
}

inline qsizetype QStringList::lastIndexOf(QLatin1String string, qsizetype from) const
{
    return QtPrivate::lastIndexOf<QString, QLatin1String>(*this, string, from);
}

#if QT_CONFIG(regularexpression)
inline QStringList &QListSpecialMethods<QString>::replaceInStrings(const QRegularExpression &rx, const QString &after)
{
    QtPrivate::QStringList_replaceInStrings(self(), rx, after);
    return *self();
}

inline QStringList QListSpecialMethods<QString>::filter(const QRegularExpression &rx) const
{
    return QtPrivate::QStringList_filter(self(), rx);
}

inline qsizetype QStringList::indexOf(const QRegularExpression &rx, qsizetype from) const
{
    return QtPrivate::QStringList_indexOf(this, rx, from);
}

inline qsizetype QStringList::lastIndexOf(const QRegularExpression &rx, qsizetype from) const
{
    return QtPrivate::QStringList_lastIndexOf(this, rx, from);
}
#endif // QT_CONFIG(regularexpression)
#endif // Q_QDOC

QT_END_NAMESPACE

#endif // QSTRINGLIST_H
