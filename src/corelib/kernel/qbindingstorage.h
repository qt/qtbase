/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef QBINDINGSTORAGE_H
#define QBINDINGSTORAGE_H

#include <QtCore/qglobal.h>
#include <QtCore/qnamespace.h>

QT_BEGIN_NAMESPACE

template <typename Class, typename T, auto Offset, auto Setter, auto Signal, auto Getter>
class QObjectCompatProperty;
struct QPropertyDelayedNotifications;
class QUntypedPropertyData;

namespace QtPrivate {

class QPropertyBindingData;
struct BindingEvaluationState;
struct CompatPropertySafePoint;
}

struct QBindingStatus
{
    QtPrivate::BindingEvaluationState *currentlyEvaluatingBinding = nullptr;
    QtPrivate::CompatPropertySafePoint *currentCompatProperty = nullptr;
    Qt::HANDLE threadId = nullptr;
    QPropertyDelayedNotifications *groupUpdateData = nullptr;
};


struct QBindingStorageData;
class Q_CORE_EXPORT QBindingStorage
{
    mutable QBindingStorageData *d = nullptr;
    QBindingStatus *bindingStatus = nullptr;

    template<typename Class, typename T, auto Offset, auto Setter, auto Signal, auto Getter>
    friend class QObjectCompatProperty;
    friend class QObjectPrivate;
    friend class QtPrivate::QPropertyBindingData;
public:
    QBindingStorage();
    ~QBindingStorage();

    bool isEmpty() { return !d; }

    void registerDependency(const QUntypedPropertyData *data) const
    {
        if (!bindingStatus->currentlyEvaluatingBinding)
            return;
        registerDependency_helper(data);
    }
    QtPrivate::QPropertyBindingData *bindingData(const QUntypedPropertyData *data) const
    {
        if (!d)
            return nullptr;
        return bindingData_helper(data);
    }

#if QT_CORE_REMOVED_SINCE(6, 2)
    void maybeUpdateBindingAndRegister(const QUntypedPropertyData *data) const { registerDependency(data); }
#endif

    QtPrivate::QPropertyBindingData *bindingData(QUntypedPropertyData *data, bool create)
    {
        if (!d && !create)
            return nullptr;
        return bindingData_helper(data, create);
    }
private:
    void clear();
    void registerDependency_helper(const QUntypedPropertyData *data) const;
#if QT_CORE_REMOVED_SINCE(6, 2)
    // ### Unused, but keep for BC
    void maybeUpdateBindingAndRegister_helper(const QUntypedPropertyData *data) const;
#endif
    QtPrivate::QPropertyBindingData *bindingData_helper(const QUntypedPropertyData *data) const;
    QtPrivate::QPropertyBindingData *bindingData_helper(QUntypedPropertyData *data, bool create);
};

QT_END_NAMESPACE

#endif // QBINDINGSTORAGE_H
