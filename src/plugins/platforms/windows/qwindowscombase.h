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

#ifndef QWINDOWSCOMBASE_H
#define QWINDOWSCOMBASE_H

#include <QtCore/qglobal.h>

#include <unknwn.h>

QT_BEGIN_NAMESPACE

// The __uuidof operator of MinGW does not work for all interfaces (for example,
// IAccessible2). Specializations of this function can be provides to work
// around this.
template <class DesiredInterface>
static IID qUuidOf() { return __uuidof(DesiredInterface); }

// Helper for implementing IUnknown::QueryInterface.
template <class DesiredInterface, class Derived>
bool qWindowsComQueryInterface(Derived *d, REFIID id, LPVOID *iface)
{
    if (id == qUuidOf<DesiredInterface>()) {
        *iface = static_cast<DesiredInterface *>(d);
        d->AddRef();
        return true;
    }
    return false;
}

// Helper for implementing IUnknown::QueryInterface for IUnknown
// in the case of multiple inheritance via the first inherited class.
template <class FirstInheritedInterface, class Derived>
bool qWindowsComQueryUnknownInterfaceMulti(Derived *d, REFIID id, LPVOID *iface)
{
    if (id == __uuidof(IUnknown)) {
        *iface = static_cast<FirstInheritedInterface *>(d);
        d->AddRef();
        return true;
    }
    return false;
}

// Helper base class to provide IUnknown methods for COM classes (single inheritance)
template <class ComInterface> class QWindowsComBase : public ComInterface
{
    Q_DISABLE_COPY_MOVE(QWindowsComBase)
public:
    explicit QWindowsComBase(ULONG initialRefCount = 1) : m_ref(initialRefCount) {}
    virtual ~QWindowsComBase() = default;

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, LPVOID *iface)
    {
        *iface = nullptr;
        return qWindowsComQueryInterface<IUnknown>(this, id, iface) || qWindowsComQueryInterface<ComInterface>(this, id, iface)
            ? S_OK : E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() { return ++m_ref; }

    ULONG STDMETHODCALLTYPE Release()
    {
        if (!--m_ref) {
            delete this;
            return 0;
        }
        return m_ref;
    }

private:
    ULONG m_ref;
};

// Clang does not consider __declspec(nothrow) as nothrow
QT_WARNING_DISABLE_CLANG("-Wmicrosoft-exception-spec")

QT_END_NAMESPACE

#endif // QWINDOWSCOMBASE_H
