/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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
#ifndef QPLATFORMACCESSIBILITY_H
#define QPLATFORMACCESSIBILITY_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <QtGui/qtguiglobal.h>

#ifndef QT_NO_ACCESSIBILITY

#include <QtCore/qobject.h>
#include <QtGui/qaccessible.h>

QT_BEGIN_NAMESPACE


class Q_GUI_EXPORT QPlatformAccessibility
{
public:
    QPlatformAccessibility();

    virtual ~QPlatformAccessibility();
    virtual void notifyAccessibilityUpdate(QAccessibleEvent *event);
    virtual void setRootObject(QObject *o);
    virtual void initialize();
    virtual void cleanup();

    inline bool isActive() const { return m_active; }
    void setActive(bool active);

private:
    bool m_active;
};

QT_END_NAMESPACE

#endif // QT_NO_ACCESSIBILITY

#endif // QPLATFORMACCESSIBILITY_H
