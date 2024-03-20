// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSCOMBASE_H
#define QWINDOWSCOMBASE_H

#include <qt_windows.h>

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

// Clang does not consider __declspec(nothrow) as nothrow
QT_WARNING_DISABLE_CLANG("-Wmicrosoft-exception-spec")
QT_WARNING_DISABLE_CLANG("-Wmissing-exception-spec")

QT_END_NAMESPACE

#endif // QWINDOWSCOMBASE_H
