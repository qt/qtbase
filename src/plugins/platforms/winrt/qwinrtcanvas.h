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
