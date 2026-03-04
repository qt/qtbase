// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:header-decls-only

#ifndef QABSTRACTNATIVEEVENTFILTER_H
#define QABSTRACTNATIVEEVENTFILTER_H

#include <QtCore/qnamespace.h>

QT_BEGIN_NAMESPACE

#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
class QAbstractNativeEventFilterPrivate;
#endif

class Q_CORE_EXPORT QAbstractNativeEventFilter
{
public:
    QAbstractNativeEventFilter();
    virtual ~QAbstractNativeEventFilter();

    virtual bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) = 0;

private:
    Q_DISABLE_COPY(QAbstractNativeEventFilter)
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    Q_DECL_UNUSED_MEMBER
    QAbstractNativeEventFilterPrivate *d = nullptr;
#endif
};

QT_END_NAMESPACE

#endif /* QABSTRACTNATIVEEVENTFILTER_H */
