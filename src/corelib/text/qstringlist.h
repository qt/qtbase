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

#ifdef Q_QDOC
class QStringList : public QList<QString>
#else
template <> struct QListSpecialMethods<QString> : QListSpecialMethodsBase<QString>
#endif
{
#ifdef Q_QDOC
public:
    using QList<QString>::QList;
    QStringList(const QString &str);
    QStringList(const QList<QString> &other);
    QStringList(QList<QString> &&other);

    QStringList &operator=(const QList<QString> &other);
    QStringList &operator=(QList<QString> &&other);
    QStringList operator+(const QStringList &other) const;
    QStringList &operator<<(const QString &str);
    QStringList &operator<<(const QStringList &other);
    QStringList &operator<<(const QList<QString> &other);
private:
#endif

public:
    inline void sort(Qt::CaseSensitivity cs = Qt::CaseSensitive)
    { QtPrivate::QStringList_sort(self(), cs); }
    inline qsizetype removeDuplicates()
    { return QtPrivate::QStringList_removeDuplicates(self()); }

    inline QString join(QStringView sep) const
    { return QtPrivate::QStringList_join(self(), sep); }
    inline QString join(QLatin1String sep) const
    { return QtPrivate::QStringList_join(*self(), sep); }
    inline QString join(QChar sep) const
    { return QtPrivate::QStringList_join(self(), &sep, 1); }

    inline QStringList filter(QStringView str, Qt::CaseSensitivity cs = Qt::CaseSensitive) const
    { return QtPrivate::QStringList_filter(self(), str, cs); }
    inline QStringList &replaceInStrings(QStringView before, QStringView after, Qt::CaseSensitivity cs = Qt::CaseSensitive)
    {
        QtPrivate::QStringList_replaceInStrings(self(), before, after, cs);
        return *self();
    }

#if QT_STRINGVIEW_LEVEL < 2
    inline QString join(const QString &sep) const
    { return QtPrivate::QStringList_join(self(), sep.constData(), sep.length()); }
    inline QStringList filter(const QString &str, Qt::CaseSensitivity cs = Qt::CaseSensitive) const
    { return QtPrivate::QStringList_filter(self(), str, cs); }
    inline QStringList &replaceInStrings(const QString &before, const QString &after, Qt::CaseSensitivity cs = Qt::CaseSensitive)
    {
        QtPrivate::QStringList_replaceInStrings(self(), before, after, cs);
        return *self();
    }
    inline QStringList &replaceInStrings(const QString &before, QStringView after, Qt::CaseSensitivity cs = Qt::CaseSensitive)
    {
        QtPrivate::QStringList_replaceInStrings(self(), before, after, cs);
        return *self();
    }
    inline QStringList &replaceInStrings(QStringView before, const QString &after, Qt::CaseSensitivity cs = Qt::CaseSensitive)
    {
        QtPrivate::QStringList_replaceInStrings(self(), before, after, cs);
        return *self();
    }
#endif
    using QListSpecialMethodsBase<QString>::contains;
    using QListSpecialMethodsBase<QString>::indexOf;
    using QListSpecialMethodsBase<QString>::lastIndexOf;

    inline bool contains(QLatin1String str, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::QStringList_contains(self(), str, cs); }
    inline bool contains(QStringView str, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::QStringList_contains(self(), str, cs); }

#if QT_STRINGVIEW_LEVEL < 2
    inline bool contains(const QString &str, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    { return QtPrivate::QStringList_contains(self(), str, cs); }
    qsizetype indexOf(const QString &str, qsizetype from = 0) const noexcept
    { return indexOf(QStringView(str), from); }
    qsizetype lastIndexOf(const QString &str, qsizetype from = -1) const noexcept
    { return lastIndexOf(QStringView(str), from); }
#endif

#if QT_CONFIG(regularexpression)
    inline QStringList filter(const QRegularExpression &re) const
    { return QtPrivate::QStringList_filter(self(), re); }
    inline QStringList &replaceInStrings(const QRegularExpression &re, const QString &after)
    {
        QtPrivate::QStringList_replaceInStrings(self(), re, after);
        return *self();
    }
    inline qsizetype indexOf(const QRegularExpression &re, qsizetype from = 0) const
    { return QtPrivate::QStringList_indexOf(self(), re, from); }
    inline qsizetype lastIndexOf(const QRegularExpression &re, qsizetype from = -1) const
    { return QtPrivate::QStringList_lastIndexOf(self(), re, from); }
#endif // QT_CONFIG(regularexpression)
};

QT_END_NAMESPACE

#endif // QSTRINGLIST_H
