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

#ifndef QWINDOWSUIAMAINPROVIDER_H
#define QWINDOWSUIAMAINPROVIDER_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiabaseprovider.h"

#include <QtCore/qpointer.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qt_windows.h>
#include <QtGui/qaccessible.h>

QT_BEGIN_NAMESPACE

// The main UI Automation class.
class QWindowsUiaMainProvider :
    public QWindowsUiaBaseProvider,
    public IRawElementProviderSimple,
    public IRawElementProviderFragment,
    public IRawElementProviderFragmentRoot
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(QWindowsUiaMainProvider)
public:
    static QWindowsUiaMainProvider *providerForAccessible(QAccessibleInterface *accessible);
    explicit QWindowsUiaMainProvider(QAccessibleInterface *a, int initialRefCount = 1);
    virtual ~QWindowsUiaMainProvider();
    static void notifyFocusChange(QAccessibleEvent *event);
    static void notifyStateChange(QAccessibleStateChangeEvent *event);
    static void notifyValueChange(QAccessibleValueChangeEvent *event);
    static void notifyNameChange(QAccessibleEvent *event);
    static void notifySelectionChange(QAccessibleEvent *event);
    static void notifyTextChange(QAccessibleEvent *event);

    // IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, LPVOID *iface) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;

    // IRawElementProviderSimple methods
    HRESULT STDMETHODCALLTYPE get_ProviderOptions(ProviderOptions *pRetVal) override;
    HRESULT STDMETHODCALLTYPE GetPatternProvider(PATTERNID idPattern, IUnknown **pRetVal) override;
    HRESULT STDMETHODCALLTYPE GetPropertyValue(PROPERTYID idProp, VARIANT *pRetVal) override;
    HRESULT STDMETHODCALLTYPE get_HostRawElementProvider(IRawElementProviderSimple **pRetVal) override;

    // IRawElementProviderFragment methods
    HRESULT STDMETHODCALLTYPE Navigate(NavigateDirection direction, IRawElementProviderFragment **pRetVal) override;
    HRESULT STDMETHODCALLTYPE GetRuntimeId(SAFEARRAY **pRetVal) override;
    HRESULT STDMETHODCALLTYPE get_BoundingRectangle(UiaRect *pRetVal) override;
    HRESULT STDMETHODCALLTYPE GetEmbeddedFragmentRoots(SAFEARRAY **pRetVal) override;
    HRESULT STDMETHODCALLTYPE SetFocus() override;
    HRESULT STDMETHODCALLTYPE get_FragmentRoot(IRawElementProviderFragmentRoot **pRetVal) override;

    // IRawElementProviderFragmentRoot methods
    HRESULT STDMETHODCALLTYPE ElementProviderFromPoint(double x, double y, IRawElementProviderFragment **pRetVal) override;
    HRESULT STDMETHODCALLTYPE GetFocus(IRawElementProviderFragment **pRetVal) override;

private:
    QString automationIdForAccessible(const QAccessibleInterface *accessible);
    ULONG m_ref;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

#endif // QWINDOWSUIAMAINPROVIDER_H
