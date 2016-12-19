/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qwin10helpers.h"

#include <QtCore/QDebug>
#include <QtCore/private/qsystemlibrary_p.h>

#if defined(Q_CC_MINGW)
#  define HAS_UI_VIEW_SETTINGS_INTEROP
// Present from MSVC2015 + SDK 10 onwards
#elif (!defined(Q_CC_MSVC) || _MSC_VER >= 1900) && NTDDI_VERSION >= 0xa000000
#  define HAS_UI_VIEW_SETTINGS_INTEROP
#  define HAS_UI_VIEW_SETTINGS
#endif

#include <inspectable.h>

#ifdef HAS_UI_VIEW_SETTINGS
#  include <windows.ui.viewmanagement.h>
#endif

#ifdef HAS_UI_VIEW_SETTINGS_INTEROP
#  include <UIViewSettingsInterop.h>
#endif

#ifndef HAS_UI_VIEW_SETTINGS_INTEROP
MIDL_INTERFACE("3694dbf9-8f68-44be-8ff5-195c98ede8a6")
IUIViewSettingsInterop : public IInspectable
{
public:
    virtual HRESULT STDMETHODCALLTYPE GetForWindow(
        __RPC__in HWND hwnd,
        __RPC__in REFIID riid,
        __RPC__deref_out_opt void **ppv) = 0;
};
#endif // !HAS_UI_VIEW_SETTINGS_INTEROP

#ifndef HAS_UI_VIEW_SETTINGS
namespace ABI {
namespace Windows {
namespace UI {
namespace ViewManagement {

enum UserInteractionMode { Mouse, Touch };

MIDL_INTERFACE("C63657F6-8850-470D-88F8-455E16EA2C26")
IUIViewSettings : public IInspectable
{
public:
    virtual HRESULT STDMETHODCALLTYPE get_UserInteractionMode(UserInteractionMode *value) = 0;
};

} // namespace ViewManagement
} // namespace UI
} // namespace Windows
} // namespace ABI
#endif // HAS_UI_VIEW_SETTINGS

QT_BEGIN_NAMESPACE

// Starting from Windows 10
struct QWindowsComBaseDLL
{
    bool init();
    bool isValid() const
    {
        return roGetActivationFactory != nullptr && windowsCreateStringReference != nullptr;
    }

    typedef HRESULT (WINAPI *RoGetActivationFactory)(HSTRING, REFIID, void **);
    typedef HRESULT (WINAPI *WindowsCreateStringReference)(PCWSTR, UINT32, HSTRING_HEADER *, HSTRING *);

    RoGetActivationFactory roGetActivationFactory = nullptr;
    WindowsCreateStringReference windowsCreateStringReference = nullptr;
};

static QWindowsComBaseDLL baseComDll;

bool QWindowsComBaseDLL::init()
{
    if (QSysInfo::windowsVersion() >= QSysInfo::WV_WINDOWS10 && !isValid()) {
        QSystemLibrary library(QStringLiteral("combase"));
        roGetActivationFactory =
            reinterpret_cast<RoGetActivationFactory>(library.resolve("RoGetActivationFactory"));
        windowsCreateStringReference =
            reinterpret_cast<WindowsCreateStringReference>(library.resolve("WindowsCreateStringReference"));
    }
    return isValid();
}

// Return tablet mode, note: Does not work for GetDesktopWindow().
bool qt_windowsIsTabletMode(HWND hwnd)
{
    bool result = false;

    if (!baseComDll.init())
        return false;

    const wchar_t uiViewSettingsId[] = L"Windows.UI.ViewManagement.UIViewSettings";
    HSTRING_HEADER uiViewSettingsIdRefHeader;
    HSTRING uiViewSettingsIdHs = nullptr;
    const UINT32 uiViewSettingsIdLen = UINT32(sizeof(uiViewSettingsId) / sizeof(uiViewSettingsId[0]) - 1);
    if (FAILED(baseComDll.windowsCreateStringReference(uiViewSettingsId, uiViewSettingsIdLen, &uiViewSettingsIdRefHeader, &uiViewSettingsIdHs)))
        return false;

    IUIViewSettingsInterop *uiViewSettingsInterop = nullptr;
    // __uuidof(IUIViewSettingsInterop);
    const GUID uiViewSettingsInteropRefId = {0x3694dbf9, 0x8f68, 0x44be,{0x8f, 0xf5, 0x19, 0x5c, 0x98, 0xed, 0xe8, 0xa6}};

    HRESULT hr = baseComDll.roGetActivationFactory(uiViewSettingsIdHs, uiViewSettingsInteropRefId,
                                                   reinterpret_cast<void **>(&uiViewSettingsInterop));
    if (FAILED(hr))
        return false;

    //  __uuidof(ABI::Windows::UI::ViewManagement::IUIViewSettings);
    const GUID uiViewSettingsRefId = {0xc63657f6, 0x8850, 0x470d,{0x88, 0xf8, 0x45, 0x5e, 0x16, 0xea, 0x2c, 0x26}};
    ABI::Windows::UI::ViewManagement::IUIViewSettings *viewSettings = nullptr;
    hr = uiViewSettingsInterop->GetForWindow(hwnd, uiViewSettingsRefId,
                                             reinterpret_cast<void **>(&viewSettings));
    if (SUCCEEDED(hr)) {
        ABI::Windows::UI::ViewManagement::UserInteractionMode currentMode;
        hr = viewSettings->get_UserInteractionMode(&currentMode);
        if (SUCCEEDED(hr))
            result = currentMode == 1; // Touch, 1
        viewSettings->Release();
    }
    uiViewSettingsInterop->Release();
    return result;
}

QT_END_NAMESPACE
