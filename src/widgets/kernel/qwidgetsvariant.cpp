/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qvariant.h"

#include "qicon.h"
#include "qsizepolicy.h"

#include "private/qvariant_p.h"
#include <private/qmetatype_p.h>

QT_BEGIN_NAMESPACE

namespace {
static void construct(QVariant::Private *x, const void *copy)
{
    switch (x->type) {
#ifndef QT_NO_ICON
    case QVariant::Icon:
        v_construct<QIcon>(x, copy);
        break;
#endif
    case QVariant::SizePolicy:
        v_construct<QSizePolicy>(x, copy);
        break;
    default:
        qWarning("Trying to construct an instance of an invalid type, type id: %i", x->type);
        x->type = QVariant::Invalid;
        return;
    }
    x->is_null = !copy;
}

static void clear(QVariant::Private *d)
{
    switch (d->type) {
#ifndef QT_NO_ICON
    case QVariant::Icon:
        v_clear<QIcon>(d);
        break;
#endif
    case QVariant::SizePolicy:
        v_clear<QSizePolicy>(d);
        break;
    default:
        Q_ASSERT(false);
        return;
    }

    d->type = QVariant::Invalid;
    d->is_null = true;
    d->is_shared = false;
}


static bool isNull(const QVariant::Private *d)
{
    switch(d->type) {
#ifndef QT_NO_ICON
    case QVariant::Icon:
        return v_cast<QIcon>(d)->isNull();
#endif
    }
    return false;
}

static bool compare(const QVariant::Private *a, const QVariant::Private *b)
{
    Q_ASSERT(a->type == b->type);
    switch(a->type) {
#ifndef QT_NO_ICON
    case QVariant::Icon:
        return false;
#endif
    case QVariant::SizePolicy:
        return *v_cast<QSizePolicy>(a) == *v_cast<QSizePolicy>(b);
    default:
        Q_ASSERT(false);
    }
    return false;
}

static bool convert(const QVariant::Private *d, int type, void *result, bool *ok)
{
    Q_UNUSED(d);
    Q_UNUSED(type);
    Q_UNUSED(result);
    if (ok)
        *ok = false;
    return false;
}

#if !defined(QT_NO_DEBUG_STREAM)
static void streamDebug(QDebug dbg, const QVariant &v)
{
    QVariant::Private *d = const_cast<QVariant::Private *>(&v.data_ptr());
    switch (d->type) {
#ifndef QT_NO_ICON
    case QVariant::Icon:
        dbg.nospace() << *v_cast<QIcon>(d);
        break;
#endif
    case QVariant::SizePolicy:
        dbg.nospace() << *v_cast<QSizePolicy>(d);
        break;
    default:
        dbg.nospace() << "QMetaType::Type(" << d->type << ")";
    }
}
#endif

static const QVariant::Handler widgets_handler = {
    construct,
    clear,
    isNull,
#ifndef QT_NO_DATASTREAM
    0,
    0,
#endif
    compare,
    convert,
    0,
#if !defined(QT_NO_DEBUG_STREAM)
    streamDebug
#else
    0
#endif
};

#define QT_IMPL_METATYPEINTERFACE_WIDGETS_TYPES(MetaTypeName, MetaTypeId, RealName) \
    QT_METATYPE_INTERFACE_INIT(RealName),

static const QMetaTypeInterface qVariantWidgetsHelper[] = {
    QT_FOR_EACH_STATIC_WIDGETS_CLASS(QT_IMPL_METATYPEINTERFACE_WIDGETS_TYPES)
};

#undef QT_IMPL_METATYPEINTERFACE_WIDGETS_TYPES

}  // namespace

extern Q_CORE_EXPORT const QMetaTypeInterface *qMetaTypeWidgetsHelper;

void qRegisterWidgetsVariant()
{
    qMetaTypeWidgetsHelper = qVariantWidgetsHelper;
    QVariantPrivate::registerHandler(QModulesPrivate::Widgets, &widgets_handler);
}
Q_CONSTRUCTOR_FUNCTION(qRegisterWidgetsVariant)

QT_END_NAMESPACE
