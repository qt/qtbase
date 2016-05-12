//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// CoreWindowNativeWindow.h: NativeWindow for managing ICoreWindow native window types.

#ifndef LIBANGLE_RENDERER_D3D_D3D11_WINRT_COREWINDOWNATIVEWINDOW_H_
#define LIBANGLE_RENDERER_D3D_D3D11_WINRT_COREWINDOWNATIVEWINDOW_H_

#include "libANGLE/renderer/d3d/d3d11/winrt/InspectableNativeWindow.h"

#include <memory>
#include <windows.graphics.display.h>

typedef ABI::Windows::Foundation::__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CWindowSizeChangedEventArgs_t IWindowSizeChangedEventHandler;
typedef ABI::Windows::Foundation::__FITypedEventHandler_2_Windows__CGraphics__CDisplay__CDisplayInformation_IInspectable_t IDisplayOrientationEventHandler;

namespace rx
{
class CoreWindowNativeWindow : public InspectableNativeWindow, public std::enable_shared_from_this<CoreWindowNativeWindow>
{
  public:
    ~CoreWindowNativeWindow();

    bool initialize(EGLNativeWindowType window, IPropertySet *propertySet) override;
    HRESULT createSwapChain(ID3D11Device *device,
                            DXGIFactory *factory,
                            DXGI_FORMAT format,
                            unsigned int width,
                            unsigned int height,
                            bool containsAlpha,
                            DXGISwapChain **swapChain) override;

  protected:
    HRESULT scaleSwapChain(const Size &windowSize, const RECT &clientRect) override;

    bool registerForSizeChangeEvents();
    void unregisterForSizeChangeEvents();

  private:
    ComPtr<ABI::Windows::UI::Core::ICoreWindow> mCoreWindow;
    ComPtr<IMap<HSTRING, IInspectable*>> mPropertyMap;
    ComPtr<ABI::Windows::Graphics::Display::IDisplayInformation> mDisplayInformation;
    EventRegistrationToken mOrientationChangedEventToken;
};

[uuid(7F924F66-EBAE-40E5-A10B-B8F35E245190)]
class CoreWindowSizeChangedHandler :
    public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, IWindowSizeChangedEventHandler, IDisplayOrientationEventHandler>
{
  public:
    CoreWindowSizeChangedHandler() { }
    HRESULT RuntimeClassInitialize(std::shared_ptr<InspectableNativeWindow> host)
    {
        if (!host)
        {
            return E_INVALIDARG;
        }

        mHost = host;
        return S_OK;
    }

    // IWindowSizeChangedEventHandler
    IFACEMETHOD(Invoke)(ABI::Windows::UI::Core::ICoreWindow *sender, ABI::Windows::UI::Core::IWindowSizeChangedEventArgs *sizeChangedEventArgs)
    {
        std::shared_ptr<InspectableNativeWindow> host = mHost.lock();
        if (host)
        {
            ABI::Windows::Foundation::Size windowSize;
            if (SUCCEEDED(sizeChangedEventArgs->get_Size(&windowSize)))
            {
                host->setNewClientSize(windowSize);
            }
        }

        return S_OK;
    }

        IFACEMETHOD(Invoke)(ABI::Windows::Graphics::Display::IDisplayInformation *displayInformation, IInspectable *)
        {
    #if defined(ANGLE_ENABLE_WINDOWS_STORE) && (WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP)
            NativeWindow::RotationFlags flags = NativeWindow::RotateNone;
            ABI::Windows::Graphics::Display::DisplayOrientations orientation;
            if (SUCCEEDED(displayInformation->get_CurrentOrientation(&orientation)))
            {
                switch (orientation)
                {
                  case ABI::Windows::Graphics::Display::DisplayOrientations_Landscape:
                    flags = NativeWindow::RotateLeft;
                    break;
                  case ABI::Windows::Graphics::Display::DisplayOrientations_LandscapeFlipped:
                    flags = NativeWindow::RotateRight;
                    break;
                  default:
                    break;
                }
            }
            std::shared_ptr<InspectableNativeWindow> host = mHost.lock();
            if (host)
            {
                host->setRotationFlags(flags);
            }
    #endif
            return S_OK;
        }


  private:
    std::weak_ptr<InspectableNativeWindow> mHost;
};

HRESULT GetCoreWindowSizeInPixels(const ComPtr<ABI::Windows::UI::Core::ICoreWindow>& coreWindow, SIZE *windowSize);
}

#endif // LIBANGLE_RENDERER_D3D_D3D11_WINRT_COREWINDOWNATIVEWINDOW_H_
