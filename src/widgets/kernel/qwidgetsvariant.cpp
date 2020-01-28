/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#include "qvariant.h"

#include "qsizepolicy.h"
#include "qwidget.h"

#include "private/qvariant_p.h"
#include <private/qmetatype_p.h>

QT_BEGIN_NAMESPACE

namespace {
static void construct(QVariant::Private *x, const void *copy)
{
    switch (x->type) {
    case QMetaType::QSizePolicy:
        v_construct<QSizePolicy>(x, copy);
        break;
    default:
        qWarning("Trying to construct an instance of an invalid type, type id: %i", x->type);
        x->type = QMetaType::UnknownType;
        return;
    }
    x->is_null = !copy;
}

static void clear(QVariant::Private *d)
{
    switch (d->type) {
    case QMetaType::QSizePolicy:
        v_clear<QSizePolicy>(d);
        break;
    default:
        Q_ASSERT(false);
        return;
    }

    d->type = QMetaType::UnknownType;
    d->is_null = true;
    d->is_shared = false;
}


static bool isNull(const QVariant::Private *)
{
    return false;
}

static bool compare(const QVariant::Private *a, const QVariant::Private *b)
{
    Q_ASSERT(a->type == b->type);
    switch(a->type) {
    case QMetaType::QSizePolicy:
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
    case QMetaType::QSizePolicy:
        dbg.nospace() << *v_cast<QSizePolicy>(d);
        break;
    default:
        dbg.nospace() << "QMetaType::Type(" << d->type << ')';
    }
}
#endif

static const QVariant::Handler widgets_handler = {
    construct,
    clear,
    isNull,
#ifndef QT_NO_DATASTREAM
    nullptr,
    nullptr,
#endif
    compare,
    convert,
    nullptr,
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
    qRegisterMetaType<QWidget*>();
    qMetaTypeWidgetsHelper = qVariantWidgetsHelper;
    QVariantPrivate::registerHandler(QModulesPrivate::Widgets, &widgets_handler);
}
Q_CONSTRUCTOR_FUNCTION(qRegisterWidgetsVariant)

QT_END_NAMESPACE
