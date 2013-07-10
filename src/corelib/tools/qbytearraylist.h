/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2014 by Southwest Research Institute (R)
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#ifndef QBYTEARRAYLIST_H
#define QBYTEARRAYLIST_H

#include <QtCore/qdatastream.h>
#include <QtCore/qlist.h>
#include <QtCore/qbytearray.h>

QT_BEGIN_NAMESPACE


typedef QListIterator<QByteArray> QByteArrayListIterator;
typedef QMutableListIterator<QByteArray> QMutableByteArrayListIterator;

class QByteArrayList : public QList<QByteArray>
{
public:
    QByteArrayList() { }
    explicit QByteArrayList(const QByteArray &i) { append(i); }
    QByteArrayList(const QList<QByteArray> &l) : QList<QByteArray>(l) { }
#ifdef Q_COMPILER_RVALUE_REFS
    QByteArrayList(QList<QByteArray> &&l) : QList<QByteArray>(qMove(l)) { }
#endif
#ifdef Q_COMPILER_INITIALIZER_LISTS
    QByteArrayList(std::initializer_list<QByteArray> args) : QList<QByteArray>(args) { }
#endif
    // compiler-generated copy/move ctor/assignment operators are ok
    // compiler-generated dtor is ok
    // inherited swap() is ok (sic!)

    // For the operators, we could just say using QList<QByteArray>::operator{=,<<},
    // but they would not return QByteArrayList&, so we need to write inline forwarders:
    QByteArrayList &operator=(const QList<QByteArray> &other)
    { QList<QByteArray>::operator=(other); return *this; }
#ifdef Q_COMPILER_RVALUE_REFS
    QByteArrayList &operator=(QList<QByteArray> &&other)
    { QList<QByteArray>::operator=(qMove(other)); return *this; }
#endif
    // if this is missing, assignment from an initializer_list is ambiguous:
#ifdef Q_COMPILER_INITIALIZER_LISTS
    QByteArrayList &operator=(std::initializer_list<QByteArray> args)
    { QByteArrayList copy(args); swap(copy); return *this; }
#endif
    QByteArrayList &operator<<(const QByteArray &str)
    { QList<QByteArray>::operator<<(str); return *this; }
    QByteArrayList &operator<<(const QList<QByteArray> &l)
    { QList<QByteArray>::operator<<(l); return *this; }

    //
    // actual functionality provided on top of what QList<QByteArray> provides starts here:
    //
    inline QByteArray join() const;
    inline QByteArray join(const QByteArray &sep) const;
    inline QByteArray join(char sep) const;
};

Q_DECLARE_TYPEINFO(QByteArrayList, Q_MOVABLE_TYPE);

namespace QtPrivate {
    QByteArray Q_CORE_EXPORT QByteArrayList_join(const QByteArrayList *that, const char *separator, int separatorLength);
}

inline QByteArray QByteArrayList::join() const
{
    return QtPrivate::QByteArrayList_join(this, 0, 0);
}

inline QByteArray QByteArrayList::join(const QByteArray &sep) const
{
    return QtPrivate::QByteArrayList_join(this, sep.constData(), sep.size());
}

inline QByteArray QByteArrayList::join(char sep) const
{
    return QtPrivate::QByteArrayList_join(this, &sep, 1);
}

inline QByteArrayList operator+(const QByteArrayList &lhs, const QByteArrayList &rhs)
{
    QByteArrayList res = lhs;
    res += rhs;
    return res;
}

#ifdef Q_COMPILER_RVALUE_REFS
inline QByteArrayList operator+(QByteArrayList &&lhs, const QByteArrayList &rhs)
{
    lhs += rhs;
    return qMove(lhs);
}
#endif

inline QByteArrayList operator+(const QByteArrayList &lhs, const QList<QByteArray> &rhs)
{
    QByteArrayList res = lhs;
    res += rhs;
    return res;
}

#ifdef Q_COMPILER_RVALUE_REFS
inline QByteArrayList operator+(QByteArrayList &&lhs, const QList<QByteArray> &rhs)
{
    lhs += rhs;
    return qMove(lhs);
}
#endif

inline QByteArrayList operator+(const QList<QByteArray> &lhs, const QByteArrayList &rhs)
{
    QByteArrayList res = lhs;
    res += rhs;
    return res;
}

#if 0 // ambiguous with QList<QByteArray>::operator+(const QList<QByteArray> &) const
#ifdef Q_COMPILER_RVALUE_REFS
inline QByteArrayList operator+(QList<QByteArray> &&lhs, const QByteArrayList &rhs)
{
    lhs += rhs;
    return qMove(lhs);
}
#endif
#endif

inline QByteArrayList& operator+=(QByteArrayList &lhs, const QList<QByteArray> &rhs)
{
    lhs.append(rhs);
    return lhs;
}

#ifndef QT_NO_DATASTREAM
inline QDataStream &operator>>(QDataStream &in, QByteArrayList &list)
{
    return operator>>(in, static_cast<QList<QByteArray> &>(list));
}
inline QDataStream &operator<<(QDataStream &out, const QByteArrayList &list)
{
    return operator<<(out, static_cast<const QList<QByteArray> &>(list));
}
#endif // QT_NO_DATASTREAM

QT_END_NAMESPACE

#endif // QBYTEARRAYLIST_H
