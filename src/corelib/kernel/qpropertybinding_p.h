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

#ifndef QPROPERTYBINDING_P_H
#define QPROPERTYBINDING_P_H

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

#include <QtCore/qglobal.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qvarlengtharray.h>
#include <memory>
#include <vector>
#include <functional>

#include "qproperty_p.h"

QT_BEGIN_NAMESPACE

struct QPropertyBindingPrivate : public QSharedData
{
    QUntypedPropertyBinding::BindingEvaluationFunction evaluationFunction;

    QPropertyObserverPointer firstObserver;
    QVarLengthArray<QPropertyObserver, 4> dependencyObservers;

    void *propertyDataPtr = nullptr;

    QPropertyBindingSourceLocation location;
    QPropertyBindingError error;

    QMetaType metaType;

    bool dirty = false;
    bool updating = false;

    QPropertyBindingPrivate(const QMetaType &metaType, QUntypedPropertyBinding::BindingEvaluationFunction evaluationFunction,
                            const QPropertyBindingSourceLocation &location)
        : evaluationFunction(std::move(evaluationFunction))
        , location(location)
        , metaType(metaType)
    {}
    virtual ~QPropertyBindingPrivate();

    void unlinkAndDeref();

    void markDirtyAndNotifyObservers();
    bool evaluateIfDirtyAndReturnTrueIfValueChanged();

    static QPropertyBindingPrivate *get(const QUntypedPropertyBinding &binding)
    { return binding.d.data(); }
};

QT_END_NAMESPACE

#endif // QPROPERTYBINDING_P_H
