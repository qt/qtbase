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

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwinrtuiabaseprovider.h"

#include <QtGui/QAccessible>
#include <QtGui/QAccessibleInterface>
#include <QtCore/QLoggingCategory>
#include <QtCore/QString>

QT_BEGIN_NAMESPACE

QWinRTUiaBaseProvider::QWinRTUiaBaseProvider(QAccessible::Id id) :
    m_id(id)
{
}

QWinRTUiaBaseProvider::~QWinRTUiaBaseProvider()
{
}

QAccessibleInterface *QWinRTUiaBaseProvider::accessibleInterface() const
{
    QAccessibleInterface *accessible = QAccessible::accessibleInterface(m_id);
    if (accessible && accessible->isValid())
        return accessible;
    return nullptr;
}

QAccessible::Id QWinRTUiaBaseProvider::id() const
{
    return m_id;
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
