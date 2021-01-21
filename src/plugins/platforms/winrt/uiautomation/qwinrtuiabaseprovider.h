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

#ifndef QWINRTUIABASEPROVIDER_H
#define QWINRTUIABASEPROVIDER_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include <QtGui/QAccessible>
#include <QtGui/QAccessibleInterface>
#include <QtCore/QPointer>

QT_BEGIN_NAMESPACE

// Base class for UI Automation providers.
class QWinRTUiaBaseProvider : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(QWinRTUiaBaseProvider)
public:
    explicit QWinRTUiaBaseProvider(QAccessible::Id id);
    virtual ~QWinRTUiaBaseProvider();

    QAccessibleInterface *accessibleInterface() const;
    QAccessible::Id id() const;

private:
    QAccessible::Id m_id;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

#endif // QWINRTUIABASEPROVIDER_H
