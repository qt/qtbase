/****************************************************************************
**
** Copyright (C) 2013 by Southwest Research Institute (R)
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
    inline QByteArrayList() { }
    inline explicit QByteArrayList(const QByteArray &i) { append(i); }
    inline QByteArrayList(const QByteArrayList &l) : QList<QByteArray>(l) { }
    inline QByteArrayList(const QList<QByteArray> &l) : QList<QByteArray>(l) { }
#ifdef Q_COMPILER_INITIALIZER_LISTS
    inline QByteArrayList(std::initializer_list<QByteArray> args) : QList<QByteArray>(args) { }
#endif

    inline QByteArray join() const;
    inline QByteArray join(const QByteArray &sep) const;
    inline QByteArray join(char sep) const;

    inline QByteArrayList &operator<<(const QByteArray &str)
    { append(str); return *this; }
    inline QByteArrayList &operator<<(const QByteArrayList &l)
    { *this += l; return *this; }
};

Q_DECLARE_TYPEINFO(QByteArrayList, Q_MOVABLE_TYPE);

namespace QtPrivate {
    QByteArray Q_CORE_EXPORT QByteArrayList_join(const QByteArrayList *that, const char *s, int l);
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
    QByteArrayList res;
    res.append( lhs );
    res.append( rhs );
    return res;
}

inline QByteArrayList& operator+=(QByteArrayList &lhs, const QByteArrayList &rhs)
{
    lhs.append( rhs );
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
