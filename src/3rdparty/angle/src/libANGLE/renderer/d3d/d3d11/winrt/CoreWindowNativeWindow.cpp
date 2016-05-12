//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// CoreWindowNativeWindow.cpp: NativeWindow for managing ICoreWindow native window types.

#include "libANGLE/renderer/d3d/d3d11/winrt/CoreWindowNativeWindow.h"

using namespace ABI::Windows::Foundation::Collections;

namespace rx
{
CoreWindowNativeWindow::~CoreWindowNativeWindow()
{
    unregisterForSizeChangeEvents();
}

bool CoreWindowNativeWindow::initialize(EGLNativeWindowType window, IPropertySet *propertySet)
{
    mOrientationChangedEventToken.value = 0;
    ComPtr<IPropertySet> props = propertySet;
    ComPtr<IInspectable> win = window;
    SIZE swapChainSize = {};
    HRESULT result = S_OK;

    // IPropertySet is an optional parameter and can be null.
    // If one is specified, cache as an IMap and read the properties
    // used for initial host initialization.
    if (propertySet)
    {
        result = props.As(&mPropertyMap);
        if (FAILED(result))
        {
            return false;
        }

        // The EGLRenderSurfaceSizeProperty is optional and may be missing. The IPropertySet
        // was prevalidated to contain the EGLNativeWindowType before being passed to
        // this host.
        result = GetOptionalSizePropertyValue(mPropertyMap, EGLRenderSurfaceSizeProperty, &swapChainSize, &mSwapChainSizeSpecified);
        if (FAILED(result))
        {
            return false;
        }

        // The EGLRenderResolutionScaleProperty is optional and may be missing. The IPropertySet
        // was prevalidated to contain the EGLNativeWindowType before being passed to
        // this host.
        result = GetOptionalSinglePropertyValue(mPropertyMap, EGLRenderResolutionScaleProperty, &mSwapChainScale, &mSwapChainScaleSpecified);
        if (FAILED(result))
        {
            return false;
        }

        if (!mSwapChainScaleSpecified)
        {
            // Default value for the scale is 1.0f
            mSwapChainScale = 1.0f;
        }

        // A EGLRenderSurfaceSizeProperty and a EGLRenderResolutionScaleProperty can't both be specified
        if (mSwapChainScaleSpecified && mSwapChainSizeSpecified)
        {
            ERR("It is invalid to specify both an EGLRenderSurfaceSizeProperty and a EGLRenderResolutionScaleProperty.");
            return false;
        }
    }

    if (SUCCEEDED(result))
    {
        result = win.As(&mCoreWindow);
    }

    if (SUCCEEDED(result))
    {
        // If a swapchain size is specfied, then the automatic resize
        // behaviors implemented by the host should be disabled.  The swapchain
        // will be still be scaled when being rendered to fit the bounds
        // of the host.
        // Scaling of the swapchain output occurs automatically because if
        // the scaling mode setting DXGI_SCALING_STRETCH on the swapchain.
        if (mSwapChainSizeSpecified)
        {
            mClientRect = { 0, 0, swapChainSize.cx, swapChainSize.cy };
        }
        else
        {
            SIZE coreWindowSize;
            result = GetCoreWindowSizeInPixels(mCoreWindow, &coreWindowSize);

            if (SUCCEEDED(result))
            {
                mClientRect = { 0, 0, static_cast<long>(coreWindowSize.cx * mSwapChainScale), static_cast<long>(coreWindowSize.cy * mSwapChainScale) };
            }
        }
    }

    if (SUCCEEDED(result))
    {
        ComPtr<ABI::Windows::Graphics::Display::IDisplayInformationStatics> displayInformation;
        result = GetActivationFactory(HStringReference(RuntimeClass_Windows_Graphics_Display_DisplayInformation).Get(), &displayInformation);
        if (SUCCEEDED(result))
        {
            result = displayInformation->GetForCurrentView(&mDisplayInformation);
        }
    }

    if (SUCCEEDED(result))
    {
        mNewClientRect = mClientRect;
        mClientRectChanged = false;
        return registerForSizeChangeEvents();
    }

    return false;
}

bool CoreWindowNativeWindow::registerForSizeChangeEvents()
{
    ComPtr<IWindowSizeChangedEventHandler> sizeChangedHandler;
    HRESULT result = Microsoft::WRL::MakeAndInitialize<CoreWindowSizeChangedHandler>(sizeChangedHandler.ReleaseAndGetAddressOf(), this->shared_from_this());
    if (SUCCEEDED(result))
    {
        result = mCoreWindow->add_SizeChanged(sizeChangedHandler.Get(), &mSizeChangedEventToken);
    }

#if defined(ANGLE_ENABLE_WINDOWS_STORE) && (WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP)
    ComPtr<IDisplayOrientationEventHandler> orientationChangedHandler;
    result = sizeChangedHandler.As(&orientationChangedHandler);
    if (SUCCEEDED(result))
    {
        result = mDisplayInformation->add_OrientationChanged(orientationChangedHandler.Get(), &mOrientationChangedEventToken);
        orientationChangedHandler->Invoke(mDisplayInformation.Get(), nullptr);
    }
#endif

    if (SUCCEEDED(result))
    {
        return true;
    }

    return false;
}

void CoreWindowNativeWindow::unregisterForSizeChangeEvents()
{
    if (mCoreWindow)
    {
        (void)mCoreWindow->remove_SizeChanged(mSizeChangedEventToken);
    }

#if defined(ANGLE_ENABLE_WINDOWS_STORE) && (WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP)
    if (mDisplayInformation)
    {
        (void)mDisplayInformation->remove_OrientationChanged(mOrientationChangedEventToken);
    }
#endif

    mSizeChangedEventToken.value = 0;
    mOrientationChangedEventToken.value = 0;
}

HRESULT CoreWindowNativeWindow::createSwapChain(ID3D11Device *device,
                                                DXGIFactory *factory,
                                                DXGI_FORMAT format,
                                                unsigned int width,
                                                unsigned int height,
                                                bool containsAlpha,
                                                DXGISwapChain **swapChain)
{
    if (device == NULL || factory == NULL || swapChain == NULL || width == 0 || height == 0)
    {
        return E_INVALIDARG;
    }

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
    swapChainDesc.Width = mRotationFlags ? height : width;
    swapChainDesc.Height = mRotationFlags ? width : height;
    swapChainDesc.Format = format;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage =
        DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_BACK_BUFFER;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.AlphaMode             = DXGI_ALPHA_MODE_UNSPECIFIED;

    *swapChain = nullptr;

    ComPtr<IDXGISwapChain1> newSwapChain;
    HRESULT result = factory->CreateSwapChainForCoreWindow(device, mCoreWindow.Get(), &swapChainDesc, nullptr, newSwapChain.ReleaseAndGetAddressOf());
    if (SUCCEEDED(result))
    {

#if 0 //(WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP) // Qt: allow Windows Phone to resize, but don't modify the backing texture in the swap chain.
        // Test if swapchain supports resize.  On Windows Phone devices, this will return DXGI_ERROR_UNSUPPORTED.  On
        // other devices DXGI_ERROR_INVALID_CALL should be returned because the combination of flags passed
        // (DXGI_SWAP_CHAIN_FLAG_NONPREROTATED | DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE) are invalid flag combinations.
        if (newSwapChain->ResizeBuffers(swapChainDesc.BufferCount, swapChainDesc.Width, swapChainDesc.Height, swapChainDesc.Format, DXGI_SWAP_CHAIN_FLAG_NONPREROTATED | DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE) == DXGI_ERROR_UNSUPPORTED)
        {
            mSupportsSwapChainResize = false;
        }
#endif // (WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP)

        result = newSwapChain.CopyTo(swapChain);
    }

    if (SUCCEEDED(result))
    {
        // If automatic swapchain resize behaviors have been disabled, then
        // unregister for the resize change events.
        if (mSupportsSwapChainResize == false)
        {
            unregisterForSizeChangeEvents();
        }
    }

    return result;
}

inline HRESULT CoreWindowNativeWindow::scaleSwapChain(const Size &windowSize, const RECT &clientRect)
{
    // We don't need to do any additional work to scale CoreWindow swapchains.
    // Using DXGI_SCALING_STRETCH to create the swapchain above does all the necessary work.
    return S_OK;
}

HRESULT GetCoreWindowSizeInPixels(const ComPtr<ABI::Windows::UI::Core::ICoreWindow>& coreWindow, SIZE *windowSize)
{
    ABI::Windows::Foundation::Rect bounds;
    HRESULT result = coreWindow->get_Bounds(&bounds);
    if (SUCCEEDED(result))
    {
        *windowSize = { ConvertDipsToPixels(bounds.Width), ConvertDipsToPixels(bounds.Height) };
    }

    return result;
}
}
