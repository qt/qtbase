// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwin10helpers.h"

#include <QtCore/qdebug.h>
#include <winstring.h>
#include <roapi.h>

#if defined(Q_CC_MINGW) || defined(Q_CC_CLANG)
#  define HAS_UI_VIEW_SETTINGS_INTEROP
// Present from MSVC2015 + SDK 10 onwards
#elif (!defined(Q_CC_MSVC) || _MSC_VER >= 1900) && WINVER >= 0x0A00
#  define HAS_UI_VIEW_SETTINGS_INTEROP
#  define HAS_UI_VIEW_SETTINGS
#endif

#include <inspectable.h>

#ifdef HAS_UI_VIEW_SETTINGS
#  include <windows.ui.viewmanagement.h>
#endif

#ifdef HAS_UI_VIEW_SETTINGS_INTEROP
#  include <uiviewsettingsinterop.h>
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

// Return tablet mode, note: Does not work for GetDesktopWindow().
bool qt_windowsIsTabletMode(HWND hwnd)
{
    bool result = false;

    const wchar_t uiViewSettingsId[] = L"Windows.UI.ViewManagement.UIViewSettings";
    HSTRING_HEADER uiViewSettingsIdRefHeader;
    HSTRING uiViewSettingsIdHs = nullptr;
    const auto uiViewSettingsIdLen = UINT32(sizeof(uiViewSettingsId) / sizeof(uiViewSettingsId[0]) - 1);
    if (FAILED(WindowsCreateStringReference(uiViewSettingsId, uiViewSettingsIdLen, &uiViewSettingsIdRefHeader, &uiViewSettingsIdHs)))
        return false;

    IUIViewSettingsInterop *uiViewSettingsInterop = nullptr;
    // __uuidof(IUIViewSettingsInterop);
    const GUID uiViewSettingsInteropRefId = {0x3694dbf9, 0x8f68, 0x44be,{0x8f, 0xf5, 0x19, 0x5c, 0x98, 0xed, 0xe8, 0xa6}};

    HRESULT hr = RoGetActivationFactory(uiViewSettingsIdHs, uiViewSettingsInteropRefId,
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
