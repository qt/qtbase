/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#ifndef QWINDOWSUIAUTILS_H
#define QWINDOWSUIAUTILS_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include <QtCore/qstring.h>
#include <QtCore/qt_windows.h>
#include <QtGui/qaccessible.h>
#include <QtGui/qwindow.h>
#include <QtCore/qrect.h>
#include <QtWindowsUIAutomationSupport/private/qwindowsuiawrapper_p.h>

QT_BEGIN_NAMESPACE

namespace QWindowsUiAutomation {

QWindow *windowForAccessible(const QAccessibleInterface *accessible);

HWND hwndForAccessible(const QAccessibleInterface *accessible);

void rectToNativeUiaRect(const QRect &rect, const QWindow *w, UiaRect *uiaRect);

void nativeUiaPointToPoint(const UiaPoint &uiaPoint, const QWindow *w, QPoint *point);

long roleToControlTypeId(QAccessible::Role role);

bool isTextUnitSeparator(TextUnit unit, const QChar &ch);

void clearVariant(VARIANT *variant);

void setVariantI4(int value, VARIANT *variant);

void setVariantBool(bool value, VARIANT *variant);

void setVariantDouble(double value, VARIANT *variant);

BSTR bStrFromQString(const QString &value);

void setVariantString(const QString &value, VARIANT *variant);

} // namespace QWindowsUiAutomation

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

#endif // QWINDOWSUIAUTILS_H
