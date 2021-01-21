/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QWINDOWSUIABASEPROVIDER_H
#define QWINDOWSUIABASEPROVIDER_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include <QtGui/qaccessible.h>
#include <QtCore/qpointer.h>

#include <qwindowscombase.h>
#include <QtWindowsUIAutomationSupport/private/qwindowsuiawrapper_p.h>

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
