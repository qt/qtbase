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

#ifndef QWINRTCANVAS_H
#define QWINRTCANVAS_H

#include <QtCore/qglobal.h>
#include <QtGui/QWindow>

#include <wrl.h>
#include <windows.ui.xaml.h>
#include <functional>

QT_BEGIN_NAMESPACE

class QWinRTCanvas:
        public Microsoft::WRL::RuntimeClass<ABI::Windows::UI::Xaml::IUIElementOverrides>
{
public:
    QWinRTCanvas(const std::function<QWindow*()> &delegateWindow);
    ~QWinRTCanvas() override = default;

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID *iface) override;
    HRESULT STDMETHODCALLTYPE GetIids(ULONG *iidCount, IID **iids) override;
    HRESULT STDMETHODCALLTYPE GetRuntimeClassName(HSTRING *className) override;
    HRESULT STDMETHODCALLTYPE GetTrustLevel(TrustLevel *trustLevel) override;
    HRESULT STDMETHODCALLTYPE OnCreateAutomationPeer(ABI::Windows::UI::Xaml::Automation::Peers::IAutomationPeer **returnValue) override;
    HRESULT STDMETHODCALLTYPE OnDisconnectVisualChildren() override;
    HRESULT STDMETHODCALLTYPE FindSubElementsForTouchTargeting(ABI::Windows::Foundation::Point point, ABI::Windows::Foundation::Rect boundingRect, ABI::Windows::Foundation::Collections::IIterable<ABI::Windows::Foundation::Collections::IIterable<ABI::Windows::Foundation::Point>*> **returnValue) override;

private:
    Microsoft::WRL::ComPtr<ABI::Windows::UI::Xaml::IUIElementOverrides> m_base;
    Microsoft::WRL::ComPtr<ABI::Windows::UI::Xaml::Controls::ICanvas> m_core;
    std::function<QWindow*()> delegate;
};

QT_END_NAMESPACE

#endif // QWINRTCANVAS_H
