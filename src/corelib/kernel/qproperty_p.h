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

#ifndef QPROPERTY_P_H
#define QPROPERTY_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qapplication_*.cpp, qwidget*.cpp and qfiledialog.cpp.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#include <qglobal.h>
#include <qvarlengtharray.h>

#include "qproperty.h"

QT_BEGIN_NAMESPACE

// This is a helper "namespace"
struct Q_AUTOTEST_EXPORT QPropertyBasePointer
{
    const QtPrivate::QPropertyBase *ptr = nullptr;

    QPropertyBindingPrivate *bindingPtr() const;

    void setObservers(QPropertyObserver *observer);
    void addObserver(QPropertyObserver *observer);
    void setFirstObserver(QPropertyObserver *observer);
    QPropertyObserverPointer firstObserver() const;

    int observerCount() const;

    template <typename T>
    static QPropertyBasePointer get(QProperty<T> &property)
    {
        return QPropertyBasePointer{&property.d.priv};
    }
};

// This is a helper "namespace"
struct QPropertyObserverPointer
{
    QPropertyObserver *ptr = nullptr;

    void unlink();

    void setBindingToMarkDirty(QPropertyBindingPrivate *binding);
    void setChangeHandler(void (*changeHandler)(QPropertyObserver *, void *));
    void setAliasedProperty(void *propertyPtr);

    void notify(QPropertyBindingPrivate *triggeringBinding, void *propertyDataPtr);
    void observeProperty(QPropertyBasePointer property);

    explicit operator bool() const { return ptr != nullptr; }

    QPropertyObserverPointer nextObserver() const { return {ptr->next.data()}; }
};

class QPropertyBindingErrorPrivate : public QSharedData
{
public:
    QPropertyBindingError::Type type = QPropertyBindingError::NoError;
    QString description;
    QPropertyBindingSourceLocation location;
};

struct BindingEvaluationState
{
    BindingEvaluationState(QPropertyBindingPrivate *binding);
    ~BindingEvaluationState();
    QPropertyBindingPrivate *binding;
    BindingEvaluationState *previousState = nullptr;
};

QT_END_NAMESPACE

#endif // QPROPERTY_P_H
