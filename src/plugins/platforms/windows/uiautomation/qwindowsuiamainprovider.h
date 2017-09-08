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

#ifndef QWINDOWSUIAMAINPROVIDER_H
#define QWINDOWSUIAMAINPROVIDER_H

#include <QtCore/QtConfig>
#ifndef QT_NO_ACCESSIBILITY

#include "qwindowsuiabaseprovider.h"

#include <QtCore/QPointer>
#include <QtCore/QSharedPointer>
#include <QtCore/qt_windows.h>
#include <QtGui/QAccessible>

QT_BEGIN_NAMESPACE

// The main UI Automation class.
class QWindowsUiaMainProvider :
    public QWindowsUiaBaseProvider,
    public IRawElementProviderSimple,
    public IRawElementProviderFragment,
    public IRawElementProviderFragmentRoot
{
    Q_OBJECT
    Q_DISABLE_COPY(QWindowsUiaMainProvider)
public:
    static QWindowsUiaMainProvider *providerForAccessible(QAccessibleInterface *accessible);
    explicit QWindowsUiaMainProvider(QAccessibleInterface *a, int initialRefCount = 1);
    virtual ~QWindowsUiaMainProvider();
    static void notifyFocusChange(QAccessibleEvent *event);
    static void notifyStateChange(QAccessibleStateChangeEvent *event);
    static void notifyValueChange(QAccessibleValueChangeEvent *event);
    static void notifyTextChange(QAccessibleEvent *event);

    // IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, LPVOID *iface);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    // IRawElementProviderSimple methods
    HRESULT STDMETHODCALLTYPE get_ProviderOptions(ProviderOptions *pRetVal);
    HRESULT STDMETHODCALLTYPE GetPatternProvider(PATTERNID idPattern, IUnknown **pRetVal);
    HRESULT STDMETHODCALLTYPE GetPropertyValue(PROPERTYID idProp, VARIANT *pRetVal);
    HRESULT STDMETHODCALLTYPE get_HostRawElementProvider(IRawElementProviderSimple **pRetVal);

    // IRawElementProviderFragment methods
    HRESULT STDMETHODCALLTYPE Navigate(NavigateDirection direction, IRawElementProviderFragment **pRetVal);
    HRESULT STDMETHODCALLTYPE GetRuntimeId(SAFEARRAY **pRetVal);
    HRESULT STDMETHODCALLTYPE get_BoundingRectangle(UiaRect *pRetVal);
    HRESULT STDMETHODCALLTYPE GetEmbeddedFragmentRoots(SAFEARRAY **pRetVal);
    HRESULT STDMETHODCALLTYPE SetFocus();
    HRESULT STDMETHODCALLTYPE get_FragmentRoot(IRawElementProviderFragmentRoot **pRetVal);

    // IRawElementProviderFragmentRoot methods
    HRESULT STDMETHODCALLTYPE ElementProviderFromPoint(double x, double y, IRawElementProviderFragment **pRetVal);
    HRESULT STDMETHODCALLTYPE GetFocus(IRawElementProviderFragment **pRetVal);

private:
    QString automationIdForAccessible(const QAccessibleInterface *accessible);
    ULONG m_ref;
};

QT_END_NAMESPACE

#endif // QT_NO_ACCESSIBILITY

#endif // QWINDOWSUIAMAINPROVIDER_H
