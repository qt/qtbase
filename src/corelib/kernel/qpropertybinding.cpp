/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "qpropertybinding_p.h"
#include "qproperty_p.h"

#include <QScopedValueRollback>
#include <QVariant>

QT_BEGIN_NAMESPACE

using namespace QtPrivate;

QPropertyBindingPrivate::~QPropertyBindingPrivate()
{
    if (firstObserver)
        firstObserver.unlink();
    if (!hasStaticObserver)
        inlineDependencyObservers.~ObserverArray(); // Explicit because of union.
}

void QPropertyBindingPrivate::unlinkAndDeref()
{
    propertyDataPtr = nullptr;
    if (!ref.deref())
        delete this;
}

void QPropertyBindingPrivate::markDirtyAndNotifyObservers()
{
    dirty = true;
    if (firstObserver)
        firstObserver.notify(this, propertyDataPtr);
    if (hasStaticObserver)
        staticObserverCallback(staticObserver);
}

bool QPropertyBindingPrivate::evaluateIfDirtyAndReturnTrueIfValueChanged()
{
    if (!dirty)
        return false;

    if (updating) {
        error = QPropertyBindingError(QPropertyBindingError::BindingLoop);
        return false;
    }

    QScopedValueRollback<bool> updateGuard(updating, true);

    BindingEvaluationState evaluationFrame(this);

    QPropertyBindingError evalError;
    QUntypedPropertyBinding::BindingEvaluationResult result;
    bool changed = false;
    if (metaType.id() == QMetaType::Bool) {
        auto propertyPtr = reinterpret_cast<QPropertyBase *>(propertyDataPtr);
        bool newValue = false;
        evalError = evaluationFunction(metaType, &newValue);
        if (evalError.type() == QPropertyBindingError::NoError) {
            if (propertyPtr->extraBit() != newValue) {
                propertyPtr->setExtraBit(newValue);
                changed = true;
            }
        }
    } else {
        QVariant resultVariant(metaType.id(), nullptr);
        evalError = evaluationFunction(metaType, resultVariant.data());
        if (evalError.type() == QPropertyBindingError::NoError) {
            int compareResult = 0;
            if (!QMetaType::compare(propertyDataPtr, resultVariant.constData(), metaType.id(), &compareResult) || compareResult != 0) {
                changed = true;
                metaType.destruct(propertyDataPtr);
                metaType.construct(propertyDataPtr, resultVariant.constData());
            }
        }
    }

    if (evalError.type() != QPropertyBindingError::NoError)
        error = evalError;

    dirty = false;
    return changed;
}

QUntypedPropertyBinding::QUntypedPropertyBinding() = default;

QUntypedPropertyBinding::QUntypedPropertyBinding(const QMetaType &metaType, QUntypedPropertyBinding::BindingEvaluationFunction function,
                                                 const QPropertyBindingSourceLocation &location)
{
    d = new QPropertyBindingPrivate(metaType, std::move(function), std::move(location));
}

QUntypedPropertyBinding::QUntypedPropertyBinding(QUntypedPropertyBinding &&other)
    : d(std::move(other.d))
{
}

QUntypedPropertyBinding::QUntypedPropertyBinding(const QUntypedPropertyBinding &other)
    : d(other.d)
{
}

QUntypedPropertyBinding &QUntypedPropertyBinding::operator=(const QUntypedPropertyBinding &other)
{
    d = other.d;
    return *this;
}

QUntypedPropertyBinding &QUntypedPropertyBinding::operator=(QUntypedPropertyBinding &&other)
{
    d = std::move(other.d);
    return *this;
}

QUntypedPropertyBinding::QUntypedPropertyBinding(QPropertyBindingPrivate *priv)
    : d(priv)
{
}

QUntypedPropertyBinding::~QUntypedPropertyBinding()
{
}

bool QUntypedPropertyBinding::isNull() const
{
    return !d;
}

QPropertyBindingError QUntypedPropertyBinding::error() const
{
    if (!d)
        return QPropertyBindingError();
    return d->bindingError();
}

QMetaType QUntypedPropertyBinding::valueMetaType() const
{
    if (!d)
        return QMetaType();
    return d->valueMetaType();
}

QT_END_NAMESPACE
