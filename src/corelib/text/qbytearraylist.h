/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
** Copyright (C) 2014 by Southwest Research Institute (R)
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include <QtCore/qlist.h>

#ifndef QBYTEARRAYLIST_H
#define QBYTEARRAYLIST_H

#include <QtCore/qbytearray.h>

QT_BEGIN_NAMESPACE

#if !defined(QT_NO_JAVA_STYLE_ITERATORS)
typedef QListIterator<QByteArray> QByteArrayListIterator;
typedef QMutableListIterator<QByteArray> QMutableByteArrayListIterator;
#endif

#ifndef Q_CLANG_QDOC
typedef QList<QByteArray> QByteArrayList;

namespace QtPrivate {
    QByteArray Q_CORE_EXPORT QByteArrayList_join(const QByteArrayList *that, const char *separator, int separatorLength);
    int Q_CORE_EXPORT QByteArrayList_indexOf(const QByteArrayList *that, const char *needle, int from);
}
#endif

#ifdef Q_CLANG_QDOC
class QByteArrayList : public QList<QByteArray>
#else
template <> struct QListSpecialMethods<QByteArray>
#endif
{
#ifndef Q_CLANG_QDOC
protected:
    ~QListSpecialMethods() = default;
#endif
public:
    inline QByteArray join() const
    { return QtPrivate::QByteArrayList_join(self(), nullptr, 0); }
    inline QByteArray join(const QByteArray &sep) const
    { return QtPrivate::QByteArrayList_join(self(), sep.constData(), sep.size()); }
    inline QByteArray join(char sep) const
    { return QtPrivate::QByteArrayList_join(self(), &sep, 1); }

    inline int indexOf(const char *needle, int from = 0) const
    { return QtPrivate::QByteArrayList_indexOf(self(), needle, from); }

private:
    typedef QList<QByteArray> Self;
    Self *self() { return static_cast<Self *>(this); }
    const Self *self() const { return static_cast<const Self *>(this); }
};

QT_END_NAMESPACE

#endif // QBYTEARRAYLIST_H
