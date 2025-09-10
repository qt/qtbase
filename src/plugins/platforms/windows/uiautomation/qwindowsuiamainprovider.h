// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSUIAMAINPROVIDER_H
#define QWINDOWSUIAMAINPROVIDER_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiabaseprovider.h"

#include <QtCore/qpointer.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qmutex.h>
#include <QtCore/qt_windows.h>
#include <QtGui/qaccessible.h>
#include <QtCore/private/qcomptr_p.h>

QT_BEGIN_NAMESPACE

// The main UI Automation class.
class QWindowsUiaMainProvider :
    public QWindowsUiaBaseProvider,
    public QComObject<IRawElementProviderSimple, IRawElementProviderFragment, IRawElementProviderFragmentRoot>
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(QWindowsUiaMainProvider)
public:
    static ComPtr<QWindowsUiaMainProvider> providerForAccessible(QAccessibleInterface *accessible);
    explicit QWindowsUiaMainProvider(QAccessibleInterface *a);
    virtual ~QWindowsUiaMainProvider();
    static void notifyFocusChange(QAccessibleEvent *event);
    static void notifyStateChange(QAccessibleStateChangeEvent *event);
    static void notifyValueChange(QAccessibleValueChangeEvent *event);
    static void notifyNameChange(QAccessibleEvent *event);
    static void notifySelectionChange(QAccessibleEvent *event);
    static void notifyTextChange(QAccessibleEvent *event);
    static void raiseNotification(QAccessibleAnnouncementEvent *event);

    // IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, LPVOID *iface) override;

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
    static void fillVariantArrayForRelation(QAccessibleInterface *accessible, QAccessible::Relation relation, VARIANT *pRetVal);
    static void setAriaProperties(QAccessibleInterface *accessible, VARIANT *pRetVal);
    static void setStyle(QAccessibleInterface *accessible, VARIANT *pRetVal);
    /** Returns the UIA style ID for a heading level from 1 to 9. */
    static int styleIdForHeadingLevel(int headingLevel);
};

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

#endif // QWINDOWSUIAMAINPROVIDER_H
