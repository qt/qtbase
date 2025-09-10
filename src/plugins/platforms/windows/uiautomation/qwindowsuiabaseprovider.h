// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSUIABASEPROVIDER_H
#define QWINDOWSUIABASEPROVIDER_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include <QtGui/qaccessible.h>
#include <QtCore/qpointer.h>

#include "qwindowsuiautomation.h"
#include <QtCore/private/qcomobject_p.h>

QT_BEGIN_NAMESPACE

class QAccessibleInterface;

// Base class for UI Automation providers.
class QWindowsUiaBaseProvider : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(QWindowsUiaBaseProvider)
public:
    explicit QWindowsUiaBaseProvider(QAccessible::Id id);
    virtual ~QWindowsUiaBaseProvider();

    QAccessibleInterface *accessibleInterface() const;
    QAccessible::Id id() const;

private:
    QAccessible::Id m_id;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

#endif // QWINDOWSUIABASEPROVIDER_H
