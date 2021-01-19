/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef QABSTRACTNATIVEEVENTFILTER_H
#define QABSTRACTNATIVEEVENTFILTER_H

#include <QtCore/qnamespace.h>

QT_BEGIN_NAMESPACE

class QAbstractNativeEventFilterPrivate;

class Q_CORE_EXPORT QAbstractNativeEventFilter
{
public:
    QAbstractNativeEventFilter();
    virtual ~QAbstractNativeEventFilter();

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    virtual bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) = 0;
#else
    virtual bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) = 0;
#endif

private:
    Q_DISABLE_COPY(QAbstractNativeEventFilter)
    QAbstractNativeEventFilterPrivate *d;
};

QT_END_NAMESPACE

#endif /* QABSTRACTNATIVEEVENTFILTER_H */
