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

#ifndef QWINRTUIAUTILS_H
#define QWINRTUIAUTILS_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include <QtCore/QString>
#include <QtCore/qt_windows.h>
#include <QtGui/QAccessible>
#include <QtGui/QAccessibleInterface>
#include <QtGui/QWindow>
#include <QtCore/QLoggingCategory>

#include <wrl.h>
#include <windows.ui.xaml.h>
#include <functional>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQpaUiAutomation)

namespace QWinRTUiAutomation {

QWindow *windowForAccessible(const QAccessibleInterface *accessible);

QAccessibleInterface *accessibleForId(QAccessible::Id id);

QAccessible::Id idForAccessible(QAccessibleInterface *accessible);

ABI::Windows::UI::Xaml::Automation::Peers::AutomationControlType roleToControlType(QAccessible::Role role);

bool isTextUnitSeparator(ABI::Windows::UI::Xaml::Automation::Text::TextUnit unit, const QChar &ch);

HRESULT qHString(const QString &str, HSTRING *returnValue);

QString hStrToQStr(const HSTRING &hStr);

} // namespace QWinRTUiAutomation

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

#endif // QWINRTUIAUTILS_H
