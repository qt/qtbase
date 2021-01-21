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
