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

#ifndef QWINRTUIAPEERVECTOR_H
#define QWINRTUIAPEERVECTOR_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include <QtCore/QString>
#include <QtCore/qt_windows.h>
#include <QtGui/QAccessible>
#include <QtGui/QAccessibleInterface>
#include <QtGui/QWindow>
#include <QVector>

#include <wrl.h>
#include <windows.ui.xaml.h>

QT_BEGIN_NAMESPACE

// Implements IVector<AutomationPeer *>
class QWinRTUiaPeerVector : public Microsoft::WRL::RuntimeClass<ABI::Windows::Foundation::Collections::IVector<ABI::Windows::UI::Xaml::Automation::Peers::AutomationPeer *>>
{
public:
    HRESULT STDMETHODCALLTYPE GetAt(quint32 index, ABI::Windows::UI::Xaml::Automation::Peers::IAutomationPeer **item) override;
    HRESULT STDMETHODCALLTYPE get_Size(quint32 *size) override;
    HRESULT STDMETHODCALLTYPE GetView(ABI::Windows::Foundation::Collections::IVectorView<ABI::Windows::UI::Xaml::Automation::Peers::AutomationPeer *> **view) override;
    HRESULT STDMETHODCALLTYPE IndexOf(ABI::Windows::UI::Xaml::Automation::Peers::IAutomationPeer *value, quint32 *index, boolean *found) override;
    HRESULT STDMETHODCALLTYPE SetAt(quint32 index, ABI::Windows::UI::Xaml::Automation::Peers::IAutomationPeer *item) override;
    HRESULT STDMETHODCALLTYPE InsertAt(quint32 index, ABI::Windows::UI::Xaml::Automation::Peers::IAutomationPeer *item) override;
    HRESULT STDMETHODCALLTYPE RemoveAt(quint32 index) override;
    HRESULT STDMETHODCALLTYPE Append(ABI::Windows::UI::Xaml::Automation::Peers::IAutomationPeer *item) override;
    HRESULT STDMETHODCALLTYPE RemoveAtEnd() override;
    HRESULT STDMETHODCALLTYPE Clear() override;
private:
    QVector<ABI::Windows::UI::Xaml::Automation::Peers::IAutomationPeer *> m_impl;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

#endif // QWINRTUIAPEERVECTOR_H
