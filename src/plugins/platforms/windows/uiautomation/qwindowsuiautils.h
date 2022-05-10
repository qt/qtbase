// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSUIAUTILS_H
#define QWINDOWSUIAUTILS_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include <QtCore/qstring.h>
#include <QtCore/qt_windows.h>
#include <QtGui/qaccessible.h>
#include <QtGui/qwindow.h>
#include <QtCore/qrect.h>
#include <QtGui/private/qwindowsuiawrapper_p.h>

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
