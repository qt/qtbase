/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef QBINDINGSTORAGE_H
#define QBINDINGSTORAGE_H

#include <QtCore/qglobal.h>
#include <QtCore/qnamespace.h>

QT_BEGIN_NAMESPACE

template <typename Class, typename T, auto Offset, auto Setter, auto Signal>
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

namespace QtPrivate {
struct QBindingStatusAccessToken;
Q_AUTOTEST_EXPORT QBindingStatus *getBindingStatus(QBindingStatusAccessToken);
}


struct QBindingStorageData;
class Q_CORE_EXPORT QBindingStorage
{
    mutable QBindingStorageData *d = nullptr;
    QBindingStatus *bindingStatus = nullptr;

    template<typename Class, typename T, auto Offset, auto Setter, auto Signal>
    friend class QObjectCompatProperty;
    friend class QObjectPrivate;
    friend class QtPrivate::QPropertyBindingData;
public:
    QBindingStorage();
    ~QBindingStorage();

    bool isEmpty() { return !d; }
    bool isValid() const noexcept { return bindingStatus; }

    const QBindingStatus *status(QtPrivate::QBindingStatusAccessToken) const;

    void registerDependency(const QUntypedPropertyData *data) const
    {
        if (!bindingStatus || !bindingStatus->currentlyEvaluatingBinding)
            return;
        registerDependency_helper(data);
    }
    QtPrivate::QPropertyBindingData *bindingData(const QUntypedPropertyData *data) const
    {
        if (!d)
            return nullptr;
        return bindingData_helper(data);
    }
    // ### Qt 7: remove unused BIC shim
    void maybeUpdateBindingAndRegister(const QUntypedPropertyData *data) const { registerDependency(data); }

    QtPrivate::QPropertyBindingData *bindingData(QUntypedPropertyData *data, bool create)
    {
        if (!d && !create)
            return nullptr;
        return bindingData_helper(data, create);
    }
private:
    void reinitAfterThreadMove();
    void clear();
    void registerDependency_helper(const QUntypedPropertyData *data) const;
    // ### Unused, but keep for BC
    void maybeUpdateBindingAndRegister_helper(const QUntypedPropertyData *data) const;
    QtPrivate::QPropertyBindingData *bindingData_helper(const QUntypedPropertyData *data) const;
    QtPrivate::QPropertyBindingData *bindingData_helper(QUntypedPropertyData *data, bool create);
};

QT_END_NAMESPACE

#endif // QBINDINGSTORAGE_H
