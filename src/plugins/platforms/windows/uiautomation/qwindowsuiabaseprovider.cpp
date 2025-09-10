// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiabaseprovider.h"
#include "qwindowsuiautils.h"
#include "qwindowscontext.h"

#include <QtGui/qaccessible.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

using namespace QWindowsUiAutomation;


QWindowsUiaBaseProvider::QWindowsUiaBaseProvider(QAccessible::Id id) :
    m_id(id)
{
}

QWindowsUiaBaseProvider::~QWindowsUiaBaseProvider()
{
}

QAccessibleInterface *QWindowsUiaBaseProvider::accessibleInterface() const
{
    QAccessibleInterface *accessible = QAccessible::accessibleInterface(m_id);
    if (accessible && accessible->isValid())
        return accessible;
    return nullptr;
}

QAccessible::Id QWindowsUiaBaseProvider::id() const
{
    return m_id;
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
