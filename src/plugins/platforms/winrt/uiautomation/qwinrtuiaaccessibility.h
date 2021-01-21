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

#ifndef QWINRTUIAACCESSIBILITY_H
#define QWINRTUIAACCESSIBILITY_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include <qpa/qplatformaccessibility.h>

QT_BEGIN_NAMESPACE

// WinRT platform accessibility implemented over UI Automation.
class QWinRTUiaAccessibility : public QPlatformAccessibility
{
public:
    explicit QWinRTUiaAccessibility();
    virtual ~QWinRTUiaAccessibility();
    static void activate();
    void notifyAccessibilityUpdate(QAccessibleEvent *event) override;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

#endif // QWINRTUIAACCESSIBILITY_H
