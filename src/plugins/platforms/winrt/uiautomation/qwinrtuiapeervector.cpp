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

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwinrtuiapeervector.h"
#include "qwinrtuiautils.h"

using namespace ABI::Windows::UI::Xaml::Automation::Peers;
using namespace ABI::Windows::Foundation::Collections;

QT_BEGIN_NAMESPACE

HRESULT QWinRTUiaPeerVector::GetAt(quint32 index, IAutomationPeer **item)
{
    if (index >= quint32(m_impl.size()))
        return E_FAIL;
    if ((*item = m_impl.at(index)))
        (*item)->AddRef();
    return S_OK;
}

HRESULT QWinRTUiaPeerVector::get_Size(quint32 *size)
{
    *size = m_impl.size();
    return S_OK;
}

HRESULT QWinRTUiaPeerVector::GetView(IVectorView<AutomationPeer *> **view)
{
    *view = nullptr;
    return E_NOTIMPL;
}

HRESULT QWinRTUiaPeerVector::IndexOf(IAutomationPeer *value, quint32 *index, boolean *found)
{
    int idx = m_impl.indexOf(value);
    if (idx > -1) {
        *index = quint32(idx);
        *found = true;
    } else {
        *found = false;
    }
    return S_OK;
}

HRESULT QWinRTUiaPeerVector::SetAt(quint32 index, IAutomationPeer *item)
{
    if (index >= quint32(m_impl.size()))
        return E_FAIL;
    if (IAutomationPeer *elem = m_impl.at(index)) {
        if (elem == item)
            return S_OK;
        else
            elem->Release();
    }
    if (item)
        item->AddRef();
    m_impl[index] = item;
    return S_OK;
}

HRESULT QWinRTUiaPeerVector::InsertAt(quint32 index, IAutomationPeer *item)
{
    if (index >= quint32(m_impl.size()))
        return E_FAIL;
    if (item)
        item->AddRef();
    m_impl.insert(index, item);
    return S_OK;
}

HRESULT QWinRTUiaPeerVector::RemoveAt(quint32 index)
{
    if (index >= quint32(m_impl.size()))
        return E_FAIL;
    if (IAutomationPeer *elem = m_impl.at(index))
        elem->Release();
    m_impl.remove(index);
    return S_OK;
}

HRESULT QWinRTUiaPeerVector::Append(IAutomationPeer *item)
{
    if (item)
        item->AddRef();
    m_impl.append(item);
    return S_OK;
}

HRESULT QWinRTUiaPeerVector::RemoveAtEnd()
{
    if (m_impl.size() == 0)
        return E_FAIL;
    if (IAutomationPeer *elem = m_impl.last())
        elem->Release();
    m_impl.removeLast();
    return S_OK;
}

HRESULT QWinRTUiaPeerVector::Clear()
{
    for (auto elem : qAsConst(m_impl))
        if (elem)
            elem->Release();
    m_impl.clear();
    return S_OK;
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
