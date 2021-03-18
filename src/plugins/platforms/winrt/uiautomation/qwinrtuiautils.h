/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
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
