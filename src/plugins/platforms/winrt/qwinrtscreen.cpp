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

#include "qwinrtscreen.h"

#include "qwinrtbackingstore.h"
#include "qwinrtinputcontext.h"
#include "qwinrtcursor.h"
#if QT_CONFIG(draganddrop)
#include "qwinrtdrag.h"
#endif
#include "qwinrtwindow.h"
#include <private/qeventdispatcher_winrt_p.h>
#include <private/qhighdpiscaling_p.h>

#include <QtCore/QLoggingCategory>
#include <QtGui/QSurfaceFormat>
#include <QtGui/QGuiApplication>
#include <qpa/qwindowsysteminterface.h>
#include <QtCore/qt_windows.h>
#include <QtCore/qfunctions_winrt.h>

#include <functional>
#include <wrl.h>
#include <windows.system.h>
#include <Windows.ApplicationModel.h>
#include <Windows.ApplicationModel.core.h>
#include <windows.devices.input.h>
#include <windows.ui.h>
#include <windows.ui.core.h>
#include <windows.ui.input.h>
#include <windows.ui.xaml.h>
#include <windows.ui.viewmanagement.h>
#include <windows.graphics.display.h>
#include <windows.foundation.h>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::ApplicationModel;
using namespace ABI::Windows::ApplicationModel::Core;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::System;
using namespace ABI::Windows::UI;
using namespace ABI::Windows::UI::Core;
using namespace ABI::Windows::UI::Input;
using namespace ABI::Windows::UI::ViewManagement;
using namespace ABI::Windows::Devices::Input;
using namespace ABI::Windows::Graphics::Display;

typedef ITypedEventHandler<CoreWindow*, WindowActivatedEventArgs*> ActivatedHandler;
typedef ITypedEventHandler<CoreWindow*, CoreWindowEventArgs*> ClosedHandler;
typedef ITypedEventHandler<CoreWindow*, CharacterReceivedEventArgs*> CharacterReceivedHandler;
typedef ITypedEventHandler<CoreWindow*, InputEnabledEventArgs*> InputEnabledHandler;
typedef ITypedEventHandler<CoreWindow*, KeyEventArgs*> KeyHandler;
typedef ITypedEventHandler<CoreWindow*, PointerEventArgs*> PointerHandler;
typedef ITypedEventHandler<CoreWindow*, WindowSizeChangedEventArgs*> SizeChangedHandler;
typedef ITypedEventHandler<CoreWindow*, VisibilityChangedEventArgs*> VisibilityChangedHandler;
typedef ITypedEventHandler<DisplayInformation*, IInspectable*> DisplayInformationHandler;
typedef ITypedEventHandler<ICorePointerRedirector*, PointerEventArgs*> RedirectHandler;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)
typedef ITypedEventHandler<ApplicationView*, IInspectable*> VisibleBoundsChangedHandler;
#endif // WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)

QT_BEGIN_NAMESPACE

struct KeyInfo {
    KeyInfo()
    {
    }

    KeyInfo(quint32 virtualKey)
        : virtualKey(virtualKey)
    {
    }

    QString text;
    quint32 virtualKey{0};
    bool isAutoRepeat{false};
};

static inline Qt::ScreenOrientations qtOrientationsFromNative(DisplayOrientations native)
{
    Qt::ScreenOrientations orientations = Qt::PrimaryOrientation;
    if (native & DisplayOrientations_Portrait)
        orientations |= Qt::PortraitOrientation;
    if (native & DisplayOrientations_PortraitFlipped)
        orientations |= Qt::InvertedPortraitOrientation;
    if (native & DisplayOrientations_Landscape)
        orientations |= Qt::LandscapeOrientation;
    if (native & DisplayOrientations_LandscapeFlipped)
        orientations |= Qt::InvertedLandscapeOrientation;
    return orientations;
}

static inline DisplayOrientations nativeOrientationsFromQt(Qt::ScreenOrientations orientation)
{
    DisplayOrientations native = DisplayOrientations_None;
    if (orientation & Qt::PortraitOrientation)
        native |= DisplayOrientations_Portrait;
    if (orientation & Qt::InvertedPortraitOrientation)
        native |= DisplayOrientations_PortraitFlipped;
    if (orientation & Qt::LandscapeOrientation)
        native |= DisplayOrientations_Landscape;
    if (orientation & Qt::InvertedLandscapeOrientation)
        native |= DisplayOrientations_LandscapeFlipped;
    return native;
}

static inline bool qIsNonPrintable(quint32 keyCode)
{
    switch (keyCode) {
    case '\b':
    case '\n':
    case '\t':
    case '\r':
    case '\v':
    case '\f':
        return true;
    default:
        return false;
    }
}

// Return Qt meta key from VirtualKey
static inline Qt::Key qKeyFromVirtual(VirtualKey key)
{
    switch (key) {

    default:
        return Qt::Key_unknown;

    // Non-printable characters
    case VirtualKey_Enter:
        return Qt::Key_Enter;
    case VirtualKey_Tab:
        return Qt::Key_Tab;
    case VirtualKey_Back:
        return Qt::Key_Backspace;

    // Modifiers
    case VirtualKey_Shift:
    case VirtualKey_LeftShift:
    case VirtualKey_RightShift:
        return Qt::Key_Shift;
    case VirtualKey_Control:
    case VirtualKey_LeftControl:
    case VirtualKey_RightControl:
        return Qt::Key_Control;
    case VirtualKey_Menu:
    case VirtualKey_LeftMenu:
    case VirtualKey_RightMenu:
        return Qt::Key_Alt;
    case VirtualKey_LeftWindows:
    case VirtualKey_RightWindows:
        return Qt::Key_Meta;

    // Toggle keys
    case VirtualKey_CapitalLock:
        return Qt::Key_CapsLock;
    case VirtualKey_NumberKeyLock:
        return Qt::Key_NumLock;
    case VirtualKey_Scroll:
        return Qt::Key_ScrollLock;

    // East-Asian language keys
    case VirtualKey_Kana:
    //case VirtualKey_Hangul: // Same enum as Kana
        return Qt::Key_Kana_Shift;
    case VirtualKey_Junja:
        return Qt::Key_Hangul_Jeonja;
    case VirtualKey_Kanji:
    //case VirtualKey_Hanja: // Same enum as Kanji
        return Qt::Key_Kanji;
    case VirtualKey_ModeChange:
        return Qt::Key_Mode_switch;
    case VirtualKey_Convert:
        return Qt::Key_Henkan;
    case VirtualKey_NonConvert:
        return Qt::Key_Muhenkan;

    // Misc. keys
    case VirtualKey_Cancel:
        return Qt::Key_Cancel;
    case VirtualKey_Clear:
        return Qt::Key_Clear;
    case VirtualKey_Application:
        return Qt::Key_ApplicationLeft;
    case VirtualKey_Sleep:
        return Qt::Key_Sleep;
    case VirtualKey_Pause:
        return Qt::Key_Pause;
    case VirtualKey_PageUp:
        return Qt::Key_PageUp;
    case VirtualKey_PageDown:
        return Qt::Key_PageDown;
    case VirtualKey_End:
        return Qt::Key_End;
    case VirtualKey_Home:
        return Qt::Key_Home;
    case VirtualKey_Left:
        return Qt::Key_Left;
    case VirtualKey_Up:
        return Qt::Key_Up;
    case VirtualKey_Right:
        return Qt::Key_Right;
    case VirtualKey_Down:
        return Qt::Key_Down;
    case VirtualKey_Select:
        return Qt::Key_Select;
    case VirtualKey_Print:
        return Qt::Key_Print;
    case VirtualKey_Execute:
        return Qt::Key_Execute;
    case VirtualKey_Insert:
        return Qt::Key_Insert;
    case VirtualKey_Delete:
        return Qt::Key_Delete;
    case VirtualKey_Help:
        return Qt::Key_Help;
    case VirtualKey_Snapshot:
        return Qt::Key_Camera;
    case VirtualKey_Escape:
        return Qt::Key_Escape;

    // Function Keys
    case VirtualKey_F1:
        return Qt::Key_F1;
    case VirtualKey_F2:
        return Qt::Key_F2;
    case VirtualKey_F3:
        return Qt::Key_F3;
    case VirtualKey_F4:
        return Qt::Key_F4;
    case VirtualKey_F5:
        return Qt::Key_F5;
    case VirtualKey_F6:
        return Qt::Key_F6;
    case VirtualKey_F7:
        return Qt::Key_F7;
    case VirtualKey_F8:
        return Qt::Key_F8;
    case VirtualKey_F9:
        return Qt::Key_F9;
    case VirtualKey_F10:
        return Qt::Key_F10;
    case VirtualKey_F11:
        return Qt::Key_F11;
    case VirtualKey_F12:
        return Qt::Key_F12;
    case VirtualKey_F13:
        return Qt::Key_F13;
    case VirtualKey_F14:
        return Qt::Key_F14;
    case VirtualKey_F15:
        return Qt::Key_F15;
    case VirtualKey_F16:
        return Qt::Key_F16;
    case VirtualKey_F17:
        return Qt::Key_F17;
    case VirtualKey_F18:
        return Qt::Key_F18;
    case VirtualKey_F19:
        return Qt::Key_F19;
    case VirtualKey_F20:
        return Qt::Key_F20;
    case VirtualKey_F21:
        return Qt::Key_F21;
    case VirtualKey_F22:
        return Qt::Key_F22;
    case VirtualKey_F23:
        return Qt::Key_F23;
    case VirtualKey_F24:
        return Qt::Key_F24;

    // Character keys
    case VirtualKey_Space:
        return Qt::Key_Space;
    case VirtualKey_Number0:
    case VirtualKey_NumberPad0:
        return Qt::Key_0;
    case VirtualKey_Number1:
    case VirtualKey_NumberPad1:
        return Qt::Key_1;
    case VirtualKey_Number2:
    case VirtualKey_NumberPad2:
        return Qt::Key_2;
    case VirtualKey_Number3:
    case VirtualKey_NumberPad3:
        return Qt::Key_3;
    case VirtualKey_Number4:
    case VirtualKey_NumberPad4:
        return Qt::Key_4;
    case VirtualKey_Number5:
    case VirtualKey_NumberPad5:
        return Qt::Key_5;
    case VirtualKey_Number6:
    case VirtualKey_NumberPad6:
        return Qt::Key_6;
    case VirtualKey_Number7:
    case VirtualKey_NumberPad7:
        return Qt::Key_7;
    case VirtualKey_Number8:
    case VirtualKey_NumberPad8:
        return Qt::Key_8;
    case VirtualKey_Number9:
    case VirtualKey_NumberPad9:
        return Qt::Key_9;
    case VirtualKey_A:
        return Qt::Key_A;
    case VirtualKey_B:
        return Qt::Key_B;
    case VirtualKey_C:
        return Qt::Key_C;
    case VirtualKey_D:
        return Qt::Key_D;
    case VirtualKey_E:
        return Qt::Key_E;
    case VirtualKey_F:
        return Qt::Key_F;
    case VirtualKey_G:
        return Qt::Key_G;
    case VirtualKey_H:
        return Qt::Key_H;
    case VirtualKey_I:
        return Qt::Key_I;
    case VirtualKey_J:
        return Qt::Key_J;
    case VirtualKey_K:
        return Qt::Key_K;
    case VirtualKey_L:
        return Qt::Key_L;
    case VirtualKey_M:
        return Qt::Key_M;
    case VirtualKey_N:
        return Qt::Key_N;
    case VirtualKey_O:
        return Qt::Key_O;
    case VirtualKey_P:
        return Qt::Key_P;
    case VirtualKey_Q:
        return Qt::Key_Q;
    case VirtualKey_R:
        return Qt::Key_R;
    case VirtualKey_S:
        return Qt::Key_S;
    case VirtualKey_T:
        return Qt::Key_T;
    case VirtualKey_U:
        return Qt::Key_U;
    case VirtualKey_V:
        return Qt::Key_V;
    case VirtualKey_W:
        return Qt::Key_W;
    case VirtualKey_X:
        return Qt::Key_X;
    case VirtualKey_Y:
        return Qt::Key_Y;
    case VirtualKey_Z:
        return Qt::Key_Z;
    case VirtualKey_Multiply:
        return Qt::Key_9;
    case VirtualKey_Add:
        return Qt::Key_9;
    case VirtualKey_Separator:
        return Qt::Key_9;
    case VirtualKey_Subtract:
        return Qt::Key_9;
    case VirtualKey_Decimal:
        return Qt::Key_9;
    case VirtualKey_Divide:
        return Qt::Key_9;

    /* Keys with no matching Qt enum (?)
    case VirtualKey_None:
    case VirtualKey_LeftButton:
    case VirtualKey_RightButton:
    case VirtualKey_MiddleButton:
    case VirtualKey_XButton1:
    case VirtualKey_XButton2:
    case VirtualKey_Final:
    case VirtualKey_Accept:*/
    }
}

// Some keys like modifiers, caps lock etc. should not be automatically repeated if the key is held down
static inline bool shouldAutoRepeat(Qt::Key key)
{
    switch (key) {
    case Qt::Key_Shift:
    case Qt::Key_Control:
    case Qt::Key_Alt:
    case Qt::Key_Meta:
    case Qt::Key_CapsLock:
    case Qt::Key_NumLock:
    case Qt::Key_ScrollLock:
        return false;
    default:
        return true;
    }
}

static inline Qt::Key qKeyFromCode(quint32 code, int mods)
{
    if (code >= 'a' && code <= 'z')
        code = toupper(code);
    if ((mods & Qt::ControlModifier) != 0) {
        if (code >= 0 && code <= 31)              // Ctrl+@..Ctrl+A..CTRL+Z..Ctrl+_
            code += '@';                       // to @..A..Z.._
    }
    return static_cast<Qt::Key>(code & 0xff);
}

typedef HRESULT (__stdcall ICoreWindow::*CoreWindowCallbackRemover)(EventRegistrationToken);
uint qHash(CoreWindowCallbackRemover key) { void *ptr = *(void **)(&key); return qHash(ptr); }
typedef HRESULT (__stdcall IDisplayInformation::*DisplayCallbackRemover)(EventRegistrationToken);
uint qHash(DisplayCallbackRemover key) { void *ptr = *(void **)(&key); return qHash(ptr); }
typedef HRESULT (__stdcall ICorePointerRedirector::*RedirectorCallbackRemover)(EventRegistrationToken);
uint qHash(RedirectorCallbackRemover key) { void *ptr = *(void **)(&key); return qHash(ptr); }
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)
typedef HRESULT (__stdcall IApplicationView2::*ApplicationView2CallbackRemover)(EventRegistrationToken);
uint qHash(ApplicationView2CallbackRemover key) { void *ptr = *(void **)(&key); return qHash(ptr); }
#endif // WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)

class QWinRTScreenPrivate
{
public:
    QTouchDevice *touchDevice;
    ComPtr<ICoreWindow> coreWindow;
    ComPtr<ICorePointerRedirector> redirect;
    ComPtr<Xaml::IDependencyObject> canvas;
    ComPtr<IApplicationView> view;
    ComPtr<IDisplayInformation> displayInformation;

    QScopedPointer<QWinRTCursor> cursor;
    QHash<quint32, QWindowSystemInterface::TouchPoint> touchPoints;
    QRectF logicalRect;
    QRectF visibleRect;
    QSurfaceFormat surfaceFormat;
    qreal logicalDpi;
    QDpi physicalDpi;
    qreal scaleFactor;
    Qt::ScreenOrientation nativeOrientation;
    Qt::ScreenOrientation orientation;
    QList<QWindow *> visibleWindows;
    QHash<Qt::Key, KeyInfo> activeKeys;
    QHash<CoreWindowCallbackRemover, EventRegistrationToken> windowTokens;
    QHash<DisplayCallbackRemover, EventRegistrationToken> displayTokens;
    QHash<RedirectorCallbackRemover, EventRegistrationToken> redirectTokens;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)
    QHash<ApplicationView2CallbackRemover, EventRegistrationToken> view2Tokens;
    ComPtr<IApplicationView2> view2;
#endif // WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)
    QAtomicPointer<QWinRTWindow> mouseGrabWindow;
    QAtomicPointer<QWinRTWindow> keyboardGrabWindow;
    QWindow *currentPressWindow = nullptr;
    QWindow *currentTargetWindow = nullptr;
};

// To be called from the XAML thread
QWinRTScreen::QWinRTScreen()
    : d_ptr(new QWinRTScreenPrivate)
{
    Q_D(QWinRTScreen);
    qCDebug(lcQpaWindows) << __FUNCTION__;
    d->orientation = Qt::PrimaryOrientation;
    d->touchDevice = nullptr;

    HRESULT hr;
    ComPtr<Xaml::IWindowStatics> windowStatics;
    hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_UI_Xaml_Window).Get(),
                                IID_PPV_ARGS(&windowStatics));
    Q_ASSERT_SUCCEEDED(hr);
    ComPtr<Xaml::IWindow> window;
    hr = windowStatics->get_Current(&window);
    Q_ASSERT_SUCCEEDED(hr);
    hr = window->Activate();
    Q_ASSERT_SUCCEEDED(hr);

    hr = window->get_CoreWindow(&d->coreWindow);
    Q_ASSERT_SUCCEEDED(hr);

    hr = d->coreWindow.As(&d->redirect);
    Q_ASSERT_SUCCEEDED(hr);

    hr = d->coreWindow->Activate();
    Q_ASSERT_SUCCEEDED(hr);

    Rect rect;
    hr = d->coreWindow->get_Bounds(&rect);
    Q_ASSERT_SUCCEEDED(hr);
    d->logicalRect = QRectF(0.0f, 0.0f, rect.Width, rect.Height);
    d->visibleRect = QRectF(0.0f, 0.0f, rect.Width, rect.Height);

    // Orientation handling
    ComPtr<IDisplayInformationStatics> displayInformationStatics;
    hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Graphics_Display_DisplayInformation).Get(),
                                IID_PPV_ARGS(&displayInformationStatics));
    Q_ASSERT_SUCCEEDED(hr);

    hr = displayInformationStatics->GetForCurrentView(&d->displayInformation);
    Q_ASSERT_SUCCEEDED(hr);

    // Set native orientation
    DisplayOrientations displayOrientation;
    hr = d->displayInformation->get_NativeOrientation(&displayOrientation);
    Q_ASSERT_SUCCEEDED(hr);
    d->nativeOrientation = static_cast<Qt::ScreenOrientation>(static_cast<int>(qtOrientationsFromNative(displayOrientation)));
    // Set initial pixel density
    onDpiChanged(nullptr, nullptr);
    d->orientation = d->nativeOrientation;

    ComPtr<IApplicationViewStatics2> applicationViewStatics;
    hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_UI_ViewManagement_ApplicationView).Get(),
                                IID_PPV_ARGS(&applicationViewStatics));
    RETURN_VOID_IF_FAILED("Could not get ApplicationViewStatics");

    hr = applicationViewStatics->GetForCurrentView(&d->view);
    RETURN_VOID_IF_FAILED("Could not access currentView");

    // Create a canvas and set it as the window content. Eventually, this should have its own method so multiple "screens" can be added
    ComPtr<Xaml::Controls::ICanvas> canvas;
    hr = RoActivateInstance(HString::MakeReference(RuntimeClass_Windows_UI_Xaml_Controls_Canvas).Get(), &canvas);
    Q_ASSERT_SUCCEEDED(hr);
    ComPtr<Xaml::IFrameworkElement> frameworkElement;
    hr = canvas.As(&frameworkElement);
    Q_ASSERT_SUCCEEDED(hr);
    hr = frameworkElement->put_Width(d->logicalRect.width());
    Q_ASSERT_SUCCEEDED(hr);
    hr = frameworkElement->put_Height(d->logicalRect.height());
    Q_ASSERT_SUCCEEDED(hr);
    ComPtr<Xaml::IUIElement> uiElement;
    hr = canvas.As(&uiElement);
    Q_ASSERT_SUCCEEDED(hr);
#if QT_CONFIG(draganddrop)
    QWinRTDrag::instance()->setUiElement(uiElement);
#endif
    hr = window->put_Content(uiElement.Get());
    Q_ASSERT_SUCCEEDED(hr);
    hr = canvas.As(&d->canvas);
    Q_ASSERT_SUCCEEDED(hr);

    d->cursor.reset(new QWinRTCursor);

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)
    hr = d->view.As(&d->view2);
    Q_ASSERT_SUCCEEDED(hr);
#endif // WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)
}

QWinRTScreen::~QWinRTScreen()
{
    Q_D(QWinRTScreen);
    qCDebug(lcQpaWindows) << __FUNCTION__ << this;

    // Unregister callbacks
    HRESULT hr;
    hr = QEventDispatcherWinRT::runOnXamlThread([this, d]() {
        HRESULT hr;
        for (QHash<CoreWindowCallbackRemover, EventRegistrationToken>::const_iterator i = d->windowTokens.begin(); i != d->windowTokens.end(); ++i) {
            hr = (d->coreWindow.Get()->*i.key())(i.value());
            Q_ASSERT_SUCCEEDED(hr);
        }
        for (QHash<DisplayCallbackRemover, EventRegistrationToken>::const_iterator i = d->displayTokens.begin(); i != d->displayTokens.end(); ++i) {
            hr = (d->displayInformation.Get()->*i.key())(i.value());
            Q_ASSERT_SUCCEEDED(hr);
        }
        for (QHash<RedirectorCallbackRemover, EventRegistrationToken>::const_iterator i = d->redirectTokens.begin(); i != d->redirectTokens.end(); ++i) {
            hr = (d->redirect.Get()->*i.key())(i.value());
            Q_ASSERT_SUCCEEDED(hr);
        }
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)
        for (QHash<ApplicationView2CallbackRemover, EventRegistrationToken>::const_iterator i = d->view2Tokens.begin(); i != d->view2Tokens.end(); ++i) {
            hr = (d->view2.Get()->*i.key())(i.value());
            Q_ASSERT_SUCCEEDED(hr);
        }
#endif // WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)
        return hr;
    });
    RETURN_VOID_IF_FAILED("Failed to unregister screen event callbacks");
}

QRect QWinRTScreen::geometry() const
{
    Q_D(const QWinRTScreen);
    return QRect(QPoint(), QSizeF(d->logicalRect.size() * d->scaleFactor).toSize());
}

QRect QWinRTScreen::availableGeometry() const
{
    Q_D(const QWinRTScreen);
    return QRectF((d->visibleRect.x() - d->logicalRect.x())* d->scaleFactor,
                  (d->visibleRect.y() - d->logicalRect.y()) * d->scaleFactor,
                  d->visibleRect.width() * d->scaleFactor,
                  d->visibleRect.height() * d->scaleFactor).toRect();
}

int QWinRTScreen::depth() const
{
    return 32;
}

QImage::Format QWinRTScreen::format() const
{
    return QImage::Format_RGB32;
}

QSizeF QWinRTScreen::physicalSize() const
{
    Q_D(const QWinRTScreen);
    return QSizeF(d->logicalRect.width() * d->scaleFactor / d->physicalDpi.first * qreal(25.4),
                  d->logicalRect.height() * d->scaleFactor / d->physicalDpi.second * qreal(25.4));
}

QDpi QWinRTScreen::logicalDpi() const
{
    Q_D(const QWinRTScreen);
    return QDpi(d->logicalDpi, d->logicalDpi);
}

qreal QWinRTScreen::pixelDensity() const
{
    Q_D(const QWinRTScreen);
    return qMax(1, qRound(d->logicalDpi / 96));
}

qreal QWinRTScreen::scaleFactor() const
{
    Q_D(const QWinRTScreen);
    return d->scaleFactor;
}

QPlatformCursor *QWinRTScreen::cursor() const
{
    Q_D(const QWinRTScreen);
    return d->cursor.data();
}

Qt::KeyboardModifiers QWinRTScreen::keyboardModifiers() const
{
    Q_D(const QWinRTScreen);

    Qt::KeyboardModifiers mods;
    CoreVirtualKeyStates mod;
    HRESULT hr = d->coreWindow->GetAsyncKeyState(VirtualKey_Shift, &mod);
    Q_ASSERT_SUCCEEDED(hr);
    if (mod & CoreVirtualKeyStates_Down)
        mods |= Qt::ShiftModifier;
    hr = d->coreWindow->GetAsyncKeyState(VirtualKey_Menu, &mod);
    Q_ASSERT_SUCCEEDED(hr);
    if (mod & CoreVirtualKeyStates_Down)
        mods |= Qt::AltModifier;
    hr = d->coreWindow->GetAsyncKeyState(VirtualKey_Control, &mod);
    Q_ASSERT_SUCCEEDED(hr);
    if (mod & CoreVirtualKeyStates_Down)
        mods |= Qt::ControlModifier;
    hr = d->coreWindow->GetAsyncKeyState(VirtualKey_LeftWindows, &mod);
    Q_ASSERT_SUCCEEDED(hr);
    if (mod & CoreVirtualKeyStates_Down) {
        mods |= Qt::MetaModifier;
    } else {
        hr = d->coreWindow->GetAsyncKeyState(VirtualKey_RightWindows, &mod);
        Q_ASSERT_SUCCEEDED(hr);
        if (mod & CoreVirtualKeyStates_Down)
            mods |= Qt::MetaModifier;
    }
    return mods;
}

Qt::ScreenOrientation QWinRTScreen::nativeOrientation() const
{
    Q_D(const QWinRTScreen);
    return d->nativeOrientation;
}

Qt::ScreenOrientation QWinRTScreen::orientation() const
{
    Q_D(const QWinRTScreen);
    return d->orientation;
}

ICoreWindow *QWinRTScreen::coreWindow() const
{
    Q_D(const QWinRTScreen);
    return d->coreWindow.Get();
}

Xaml::IDependencyObject *QWinRTScreen::canvas() const
{
    Q_D(const QWinRTScreen);
    return d->canvas.Get();
}

void QWinRTScreen::initialize()
{
    Q_D(QWinRTScreen);
    HRESULT hr;
    hr = d->coreWindow->add_KeyDown(Callback<KeyHandler>(this, &QWinRTScreen::onKeyDown).Get(), &d->windowTokens[&ICoreWindow::remove_KeyDown]);
    Q_ASSERT_SUCCEEDED(hr);
    hr = d->coreWindow->add_KeyUp(Callback<KeyHandler>(this, &QWinRTScreen::onKeyUp).Get(), &d->windowTokens[&ICoreWindow::remove_KeyUp]);
    Q_ASSERT_SUCCEEDED(hr);
    hr = d->coreWindow->add_CharacterReceived(Callback<CharacterReceivedHandler>(this, &QWinRTScreen::onCharacterReceived).Get(), &d->windowTokens[&ICoreWindow::remove_CharacterReceived]);
    Q_ASSERT_SUCCEEDED(hr);
    hr = d->coreWindow->add_PointerEntered(Callback<PointerHandler>(this, &QWinRTScreen::onPointerEntered).Get(), &d->windowTokens[&ICoreWindow::remove_PointerEntered]);
    Q_ASSERT_SUCCEEDED(hr);
    hr = d->coreWindow->add_PointerExited(Callback<PointerHandler>(this, &QWinRTScreen::onPointerExited).Get(), &d->windowTokens[&ICoreWindow::remove_PointerExited]);
    Q_ASSERT_SUCCEEDED(hr);
    hr = d->coreWindow->add_PointerMoved(Callback<PointerHandler>(this, &QWinRTScreen::onPointerUpdated).Get(), &d->windowTokens[&ICoreWindow::remove_PointerMoved]);
    Q_ASSERT_SUCCEEDED(hr);
    hr = d->coreWindow->add_PointerPressed(Callback<PointerHandler>(this, &QWinRTScreen::onPointerUpdated).Get(), &d->windowTokens[&ICoreWindow::remove_PointerPressed]);
    Q_ASSERT_SUCCEEDED(hr);
    hr = d->coreWindow->add_PointerReleased(Callback<PointerHandler>(this, &QWinRTScreen::onPointerUpdated).Get(), &d->windowTokens[&ICoreWindow::remove_PointerReleased]);
    Q_ASSERT_SUCCEEDED(hr);
    hr = d->coreWindow->add_PointerWheelChanged(Callback<PointerHandler>(this, &QWinRTScreen::onPointerUpdated).Get(), &d->windowTokens[&ICoreWindow::remove_PointerWheelChanged]);
    Q_ASSERT_SUCCEEDED(hr);
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)
    hr = d->view2->add_VisibleBoundsChanged(Callback<VisibleBoundsChangedHandler>(this, &QWinRTScreen::onWindowSizeChanged).Get(), &d->view2Tokens[&IApplicationView2::remove_VisibleBoundsChanged]);
    Q_ASSERT_SUCCEEDED(hr);
#else
    hr = d->coreWindow->add_SizeChanged(Callback<SizeChangedHandler>(this, &QWinRTScreen::onWindowSizeChanged).Get(), &d->windowTokens[&ICoreWindow::remove_SizeChanged]);
    Q_ASSERT_SUCCEEDED(hr)
#endif // WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)

    hr = d->coreWindow->add_Activated(Callback<ActivatedHandler>(this, &QWinRTScreen::onActivated).Get(), &d->windowTokens[&ICoreWindow::remove_Activated]);
    Q_ASSERT_SUCCEEDED(hr);
    hr = d->coreWindow->add_Closed(Callback<ClosedHandler>(this, &QWinRTScreen::onClosed).Get(), &d->windowTokens[&ICoreWindow::remove_Closed]);
    Q_ASSERT_SUCCEEDED(hr);
    hr = d->coreWindow->add_VisibilityChanged(Callback<VisibilityChangedHandler>(this, &QWinRTScreen::onVisibilityChanged).Get(), &d->windowTokens[&ICoreWindow::remove_VisibilityChanged]);
    Q_ASSERT_SUCCEEDED(hr);
    hr = d->displayInformation->add_OrientationChanged(Callback<DisplayInformationHandler>(this, &QWinRTScreen::onOrientationChanged).Get(), &d->displayTokens[&IDisplayInformation::remove_OrientationChanged]);
    Q_ASSERT_SUCCEEDED(hr);
    hr = d->displayInformation->add_DpiChanged(Callback<DisplayInformationHandler>(this, &QWinRTScreen::onDpiChanged).Get(), &d->displayTokens[&IDisplayInformation::remove_DpiChanged]);
    Q_ASSERT_SUCCEEDED(hr);
    onOrientationChanged(nullptr, nullptr);
    onVisibilityChanged(nullptr, nullptr);

    hr = d->redirect->add_PointerRoutedReleased(Callback<RedirectHandler>(this, &QWinRTScreen::onRedirectReleased).Get(), &d->redirectTokens[&ICorePointerRedirector::remove_PointerRoutedReleased]);
    Q_ASSERT_SUCCEEDED(hr);
}

void QWinRTScreen::setCursorRect(const QRectF &cursorRect)
{
    mCursorRect = cursorRect;
}

void QWinRTScreen::setKeyboardRect(const QRectF &keyboardRect)
{
    Q_D(QWinRTScreen);
    QRectF visibleRectF;
    HRESULT hr;
    Rect windowSize;

    hr = d->coreWindow->get_Bounds(&windowSize);
    if (FAILED(hr)) {
        qErrnoWarning(hr, "Failed to get window bounds");
        return;
    }
    d->logicalRect = QRectF(windowSize.X, windowSize.Y, windowSize.Width, windowSize.Height);
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)
    Rect visibleRect;
    hr = d->view2->get_VisibleBounds(&visibleRect);
    if (FAILED(hr)) {
        qErrnoWarning(hr, "Failed to get window visible bounds");
        return;
    }
    visibleRectF = QRectF(visibleRect.X, visibleRect.Y, visibleRect.Width, visibleRect.Height);
#else
    visibleRectF = d->logicalRect;
#endif // WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)
    // if keyboard is snapped to the bottom of the screen and would cover the cursor the content is
    // moved up to make it visible
    if (keyboardRect.intersects(mCursorRect)
            && qFuzzyCompare(geometry().height(), keyboardRect.y() + keyboardRect.height())) {
        visibleRectF.moveTop(visibleRectF.top() - keyboardRect.height() / d->scaleFactor);
    }
    d->visibleRect = visibleRectF;

    qCDebug(lcQpaWindows) << __FUNCTION__ << d->visibleRect;
    QWindowSystemInterface::handleScreenGeometryChange(screen(), geometry(), availableGeometry());
    QPlatformScreen::resizeMaximizedWindows();
    handleExpose();
}

QWindow *QWinRTScreen::topWindow() const
{
    Q_D(const QWinRTScreen);
    return d->visibleWindows.isEmpty() ? 0 : d->visibleWindows.first();
}

QWindow *QWinRTScreen::windowAt(const QPoint &pos)
{
    Q_D(const QWinRTScreen);
    for (auto w : qAsConst(d->visibleWindows)) {
        if (w->geometry().contains(pos))
            return w;
    }
    qCDebug(lcQpaWindows) << __FUNCTION__ << ": No window found at:" << pos;
    return nullptr;
}

void QWinRTScreen::addWindow(QWindow *window)
{
    Q_D(QWinRTScreen);
    qCDebug(lcQpaWindows) << __FUNCTION__ << window;
    if (window == topWindow() || window->surfaceClass() == QSurface::Offscreen)
        return;

    d->visibleWindows.prepend(window);
    const Qt::WindowType type = window->type();
    if (type != Qt::Popup && type != Qt::ToolTip && type != Qt::Tool) {
        updateWindowTitle(window->title());
        QWindowSystemInterface::handleWindowActivated(window, Qt::OtherFocusReason);
    }

    handleExpose();
    QWindowSystemInterface::flushWindowSystemEvents(QEventLoop::ExcludeUserInputEvents);

#if QT_CONFIG(draganddrop)
    QWinRTDrag::instance()->setDropTarget(window);
#endif
}

void QWinRTScreen::removeWindow(QWindow *window)
{
    Q_D(QWinRTScreen);
    qCDebug(lcQpaWindows) << __FUNCTION__ << window;

    const bool wasTopWindow = window == topWindow();
    if (!d->visibleWindows.removeAll(window))
        return;

    const Qt::WindowType type = window->type();
    if (wasTopWindow && type != Qt::Popup && type != Qt::ToolTip && type != Qt::Tool)
        QWindowSystemInterface::handleWindowActivated(nullptr, Qt::OtherFocusReason);
    handleExpose();
    QWindowSystemInterface::flushWindowSystemEvents(QEventLoop::ExcludeUserInputEvents);
#if QT_CONFIG(draganddrop)
    if (wasTopWindow)
        QWinRTDrag::instance()->setDropTarget(topWindow());
#endif
}

void QWinRTScreen::raise(QWindow *window)
{
    Q_D(QWinRTScreen);
    d->visibleWindows.removeAll(window);
    addWindow(window);
}

void QWinRTScreen::lower(QWindow *window)
{
    Q_D(QWinRTScreen);
    const bool wasTopWindow = window == topWindow();
    if (wasTopWindow && d->visibleWindows.size() == 1)
        return;
    if (window->surfaceClass() == QSurface::Offscreen)
        return;
    d->visibleWindows.removeAll(window);
    d->visibleWindows.append(window);
    if (wasTopWindow)
        QWindowSystemInterface::handleWindowActivated(window, Qt::OtherFocusReason);
    handleExpose();
}

bool QWinRTScreen::setMouseGrabWindow(QWinRTWindow *window, bool grab)
{
    Q_D(QWinRTScreen);
    qCDebug(lcQpaWindows) << __FUNCTION__ << window
                          << "(" << window->window()->objectName() << "):" << grab;

    if (!grab || window == nullptr)
        d->mouseGrabWindow = nullptr;
    else if (d->mouseGrabWindow != window)
        d->mouseGrabWindow = window;
    return grab;
}

QWinRTWindow *QWinRTScreen::mouseGrabWindow() const
{
    Q_D(const QWinRTScreen);
    return d->mouseGrabWindow;
}

bool QWinRTScreen::setKeyboardGrabWindow(QWinRTWindow *window, bool grab)
{
    Q_D(QWinRTScreen);
    qCDebug(lcQpaWindows) << __FUNCTION__ << window
                          << "(" << window->window()->objectName() << "):" << grab;

    if (!grab || window == nullptr)
        d->keyboardGrabWindow = nullptr;
    else if (d->keyboardGrabWindow != window)
        d->keyboardGrabWindow = window;
    return grab;
}

QWinRTWindow *QWinRTScreen::keyboardGrabWindow() const
{
    Q_D(const QWinRTScreen);
    return d->keyboardGrabWindow;
}

void QWinRTScreen::updateWindowTitle(const QString &title)
{
    Q_D(QWinRTScreen);

    HStringReference titleRef(reinterpret_cast<LPCWSTR>(title.utf16()), title.length());
    HRESULT hr = d->view->put_Title(titleRef.Get());
    RETURN_VOID_IF_FAILED("Unable to set window title");
}

void QWinRTScreen::handleExpose()
{
    Q_D(QWinRTScreen);
    if (d->visibleWindows.isEmpty())
        return;
    QList<QWindow *>::const_iterator it = d->visibleWindows.constBegin();
    QWindowSystemInterface::handleExposeEvent(*it, geometry());
    while (++it != d->visibleWindows.constEnd())
        QWindowSystemInterface::handleExposeEvent(*it, QRegion());
}

HRESULT QWinRTScreen::onKeyDown(ABI::Windows::UI::Core::ICoreWindow *, ABI::Windows::UI::Core::IKeyEventArgs *args)
{
    Q_D(QWinRTScreen);
    VirtualKey virtualKey;
    HRESULT hr = args->get_VirtualKey(&virtualKey);
    Q_ASSERT_SUCCEEDED(hr);
    CorePhysicalKeyStatus status;
    hr = args->get_KeyStatus(&status);
    Q_ASSERT_SUCCEEDED(hr);

    Qt::Key key = qKeyFromVirtual(virtualKey);

    const bool wasPressed =  d->activeKeys.contains(key);
    if (wasPressed) {
        if (!shouldAutoRepeat(key))
            return S_OK;

        d->activeKeys[key].isAutoRepeat = true;
        // If the key was pressed before trigger a key release before the next key press
        QWindowSystemInterface::handleExtendedKeyEvent(
                    topWindow(),
                    QEvent::KeyRelease,
                    key,
                    keyboardModifiers(),
                    !status.ScanCode ? -1 : status.ScanCode,
                    virtualKey,
                    0,
                    QString(),
                    d->activeKeys.value(key).isAutoRepeat);
    } else {
        d->activeKeys.insert(key, KeyInfo(virtualKey));
    }

    // Defer character key presses to onCharacterReceived
    if (key == Qt::Key_unknown || (key >= Qt::Key_Space && key <= Qt::Key_ydiaeresis))
        return S_OK;

    Qt::KeyboardModifiers modifiers = keyboardModifiers();
    // If the key actually pressed is a modifier key, then we remove its modifier key from the
    // state, since a modifier-key can't have itself as a modifier (see qwindowskeymapper.cpp)
    if (key == Qt::Key_Control)
        modifiers = modifiers ^ Qt::ControlModifier;
    else if (key == Qt::Key_Shift)
        modifiers = modifiers ^ Qt::ShiftModifier;
    else if (key == Qt::Key_Alt)
        modifiers = modifiers ^ Qt::AltModifier;

    QWindowSystemInterface::handleExtendedKeyEvent(
                topWindow(),
                QEvent::KeyPress,
                key,
                modifiers,
                !status.ScanCode ? -1 : status.ScanCode,
                virtualKey,
                0,
                QString(),
                d->activeKeys.value(key).isAutoRepeat);
    return S_OK;
}

HRESULT QWinRTScreen::onKeyUp(ABI::Windows::UI::Core::ICoreWindow *, ABI::Windows::UI::Core::IKeyEventArgs *args)
{
    Q_D(QWinRTScreen);
    VirtualKey virtualKey;
    HRESULT hr = args->get_VirtualKey(&virtualKey);
    Q_ASSERT_SUCCEEDED(hr);
    CorePhysicalKeyStatus status;
    hr = args->get_KeyStatus(&status);
    Q_ASSERT_SUCCEEDED(hr);

    Qt::Key key = qKeyFromVirtual(virtualKey);
    const KeyInfo info = d->activeKeys.take(key);
    QWindowSystemInterface::handleExtendedKeyEvent(
                topWindow(),
                QEvent::KeyRelease,
                key,
                keyboardModifiers(),
                !status.ScanCode ? -1 : status.ScanCode,
                virtualKey,
                0,
                info.text,
                false); // The final key release does not have autoRepeat set on Windows
    return S_OK;
}

HRESULT QWinRTScreen::onCharacterReceived(ICoreWindow *, ICharacterReceivedEventArgs *args)
{
    Q_D(QWinRTScreen);
    quint32 keyCode;
    HRESULT hr = args->get_KeyCode(&keyCode);
    Q_ASSERT_SUCCEEDED(hr);
    CorePhysicalKeyStatus status;
    hr = args->get_KeyStatus(&status);
    Q_ASSERT_SUCCEEDED(hr);

    // Don't generate character events for non-printables; the meta key stage is enough
    if (qIsNonPrintable(keyCode))
        return S_OK;

    const Qt::KeyboardModifiers modifiers = keyboardModifiers();
    const Qt::Key key = qKeyFromCode(keyCode, modifiers);
    const QString text = QChar(keyCode);
    KeyInfo &info = d->activeKeys[key];
    info.text = text;
    QWindowSystemInterface::handleExtendedKeyEvent(
                topWindow(),
                QEvent::KeyPress,
                key,
                modifiers,
                !status.ScanCode ? -1 : status.ScanCode,
                info.virtualKey,
                0,
                text,
                info.isAutoRepeat);
    return S_OK;
}

HRESULT QWinRTScreen::onPointerEntered(ICoreWindow *, IPointerEventArgs *args)
{
    Q_D(QWinRTScreen);

    ComPtr<IPointerPoint> pointerPoint;
    if (SUCCEEDED(args->get_CurrentPoint(&pointerPoint))) {
        // Assumes full-screen window
        Point point;
        pointerPoint->get_Position(&point);
        QPoint pos(point.X * d->scaleFactor, point.Y * d->scaleFactor);

        d->currentTargetWindow = topWindow();
        if (d->mouseGrabWindow)
            d->currentTargetWindow = d->mouseGrabWindow.load()->window();

        QWindowSystemInterface::handleEnterEvent(d->currentTargetWindow, pos, pos);
    }
    return S_OK;
}

HRESULT QWinRTScreen::onPointerExited(ICoreWindow *, IPointerEventArgs *args)
{
    Q_D(QWinRTScreen);

    ComPtr<IPointerPoint> pointerPoint;
    if (FAILED(args->get_CurrentPoint(&pointerPoint)))
        return E_INVALIDARG;

    quint32 id;
    if (FAILED(pointerPoint->get_PointerId(&id)))
        return E_INVALIDARG;

    d->touchPoints.remove(id);

    if (d->mouseGrabWindow)
        d->currentTargetWindow = d->mouseGrabWindow.load()->window();

    QWindowSystemInterface::handleLeaveEvent(d->currentTargetWindow);
    d->currentTargetWindow = nullptr;
    return S_OK;
}

// Required for qwinrtdrag.cpp
ComPtr<IPointerPoint> qt_winrt_lastPointerPoint;

HRESULT QWinRTScreen::onPointerUpdated(ICoreWindow *, IPointerEventArgs *args)
{
    Q_D(QWinRTScreen);
    ComPtr<IPointerPoint> pointerPoint;
    if (FAILED(args->get_CurrentPoint(&pointerPoint)))
        return E_INVALIDARG;

    qt_winrt_lastPointerPoint = pointerPoint;
    // Common traits - point, modifiers, properties
    Point point;
    pointerPoint->get_Position(&point);
    const QPointF pos(point.X * d->scaleFactor, point.Y * d->scaleFactor);
    QPointF localPos = pos;

    const QPoint posPoint = pos.toPoint();
    QWindow *windowUnderPointer = windowAt(QHighDpiScaling::mapPositionFromNative(posPoint, this));
    d->currentTargetWindow = windowUnderPointer;

    if (d->mouseGrabWindow)
        d->currentTargetWindow = d->mouseGrabWindow.load()->window();

    if (d->currentTargetWindow) {
        const QPointF globalPosDelta = pos - posPoint;
        localPos = d->currentTargetWindow->mapFromGlobal(posPoint) + globalPosDelta;
    }

    VirtualKeyModifiers modifiers;
    args->get_KeyModifiers(&modifiers);
    Qt::KeyboardModifiers mods;
    if (modifiers & VirtualKeyModifiers_Control)
        mods |= Qt::ControlModifier;
    if (modifiers & VirtualKeyModifiers_Menu)
        mods |= Qt::AltModifier;
    if (modifiers & VirtualKeyModifiers_Shift)
        mods |= Qt::ShiftModifier;
    if (modifiers & VirtualKeyModifiers_Windows)
        mods |= Qt::MetaModifier;

    ComPtr<IPointerPointProperties> properties;
    if (FAILED(pointerPoint->get_Properties(&properties)))
        return E_INVALIDARG;

    ComPtr<IPointerDevice> pointerDevice;
    HRESULT hr = pointerPoint->get_PointerDevice(&pointerDevice);
    RETURN_OK_IF_FAILED("Failed to get pointer device.");

    PointerDeviceType pointerDeviceType;
    hr = pointerDevice->get_PointerDeviceType(&pointerDeviceType);
    RETURN_OK_IF_FAILED("Failed to get pointer device type.");

    switch (pointerDeviceType) {
    case PointerDeviceType_Mouse: {
        qint32 delta;
        properties->get_MouseWheelDelta(&delta);
        if (delta) {
            boolean isHorizontal;
            properties->get_IsHorizontalMouseWheel(&isHorizontal);
            QPoint angleDelta(isHorizontal ? delta : 0, isHorizontal ? 0 : delta);
            QWindowSystemInterface::handleWheelEvent(d->currentTargetWindow, localPos, pos, QPoint(), angleDelta, mods);
            break;
        }

        boolean isPressed;
        Qt::MouseButtons buttons = Qt::NoButton;
        properties->get_IsLeftButtonPressed(&isPressed);
        if (isPressed)
            buttons |= Qt::LeftButton;

        properties->get_IsMiddleButtonPressed(&isPressed);
        if (isPressed)
            buttons |= Qt::MiddleButton;

        properties->get_IsRightButtonPressed(&isPressed);
        if (isPressed)
            buttons |= Qt::RightButton;

        properties->get_IsXButton1Pressed(&isPressed);
        if (isPressed)
            buttons |= Qt::XButton1;

        properties->get_IsXButton2Pressed(&isPressed);
        if (isPressed)
            buttons |= Qt::XButton2;

        // In case of a mouse grab we have to store the target of a press event
        // to be able to send one additional release event to this target when the mouse
        // button is released. This is a similar approach to AutoMouseCapture in the
        // windows qpa backend. Otherwise the release might not be propagated and the original
        // press event receiver considers a button to still be pressed, as in Qt Quick Controls 1
        // menus.
        if (buttons != Qt::NoButton && d->currentPressWindow == nullptr && !d->mouseGrabWindow)
            d->currentPressWindow = windowUnderPointer;
        if (buttons == Qt::NoButton && d->currentPressWindow && d->mouseGrabWindow) {
            const QPointF globalPosDelta = pos - posPoint;
            const QPointF localPressPos = d->currentPressWindow->mapFromGlobal(posPoint) + globalPosDelta;

            QWindowSystemInterface::handleMouseEvent(d->currentPressWindow, localPressPos, pos, buttons, mods);
            d->currentPressWindow = nullptr;
        }
        // If the mouse button is released outside of a window, targetWindow is 0, but the event
        // has to be delivered to the window, that initially received the mouse press. Do not reset
        // d->currentTargetWindow though, as it is used (and reset) in onPointerExited.
        if (buttons == Qt::NoButton && d->currentPressWindow && !d->currentTargetWindow) {
            d->currentTargetWindow = d->currentPressWindow;
            d->currentPressWindow = nullptr;
        }

        QWindowSystemInterface::handleMouseEvent(d->currentTargetWindow, localPos, pos, buttons, mods);

        break;
    }
    case PointerDeviceType_Pen:
    case PointerDeviceType_Touch: {
        if (!d->touchDevice) {
            d->touchDevice = new QTouchDevice;
            d->touchDevice->setName(QStringLiteral("WinRTTouchScreen"));
            d->touchDevice->setType(QTouchDevice::TouchScreen);
            d->touchDevice->setCapabilities(QTouchDevice::Position | QTouchDevice::Area | QTouchDevice::Pressure | QTouchDevice::NormalizedPosition);
            QWindowSystemInterface::registerTouchDevice(d->touchDevice);
        }

        quint32 id;
        pointerPoint->get_PointerId(&id);

        Rect area;
        properties->get_ContactRect(&area);

        float pressure;
        properties->get_Pressure(&pressure);

        boolean isPressed;
        pointerPoint->get_IsInContact(&isPressed);

        // Devices like the Hololens set a static pressure of 0.0 or 0.5
        // (depending on the image) independent of the pressed state.
        // In those cases we need to synthesize the pressure value. To our
        // knowledge this does not apply to pens
        if (pointerDeviceType == PointerDeviceType_Touch && (pressure == 0.0f || pressure == 0.5f))
            pressure = isPressed ? 1. : 0.;

        const QRectF areaRect(area.X * d->scaleFactor, area.Y * d->scaleFactor,
                        area.Width * d->scaleFactor, area.Height * d->scaleFactor);

        QHash<quint32, QWindowSystemInterface::TouchPoint>::iterator it = d->touchPoints.find(id);
        if (it == d->touchPoints.end()) {
            it = d->touchPoints.insert(id, QWindowSystemInterface::TouchPoint());
            it.value().id = id;
        }

        const bool wasPressEvent = isPressed && it.value().pressure == 0.;
        if (wasPressEvent)
            it.value().state = Qt::TouchPointPressed;
        else if (!isPressed && it.value().pressure > 0.)
            it.value().state = Qt::TouchPointReleased;
        else if (it.value().area == areaRect)
            it.value().state = Qt::TouchPointStationary;
        else
            it.value().state = Qt::TouchPointMoved;

        it.value().area = areaRect;
        it.value().normalPosition = QPointF(point.X/d->logicalRect.width(), point.Y/d->logicalRect.height());
        it.value().pressure = pressure;

        QWindowSystemInterface::handleTouchEvent(d->currentTargetWindow, d->touchDevice, d->touchPoints.values(), mods);
        if (wasPressEvent)
            it.value().state = Qt::TouchPointStationary;

        // Fall-through for pen to generate tablet event
        if (pointerDeviceType != PointerDeviceType_Pen)
            break;

        boolean isEraser;
        properties->get_IsEraser(&isEraser);
        int pointerType = isEraser ? 3 : 1;

        float xTilt;
        properties->get_XTilt(&xTilt);

        float yTilt;
        properties->get_YTilt(&yTilt);

        float rotation;
        properties->get_Twist(&rotation);

        QWindowSystemInterface::handleTabletEvent(d->currentTargetWindow, isPressed, pos, pos, 0,
                                                  pointerType, pressure, xTilt, yTilt,
                                                  0, rotation, 0, id, mods);

        break;
    }
    }

    return S_OK;
}

HRESULT QWinRTScreen::onActivated(ICoreWindow *, IWindowActivatedEventArgs *args)
{
    Q_D(QWinRTScreen);
    qCDebug(lcQpaWindows) << __FUNCTION__;

    CoreWindowActivationState activationState;
    args->get_WindowActivationState(&activationState);
    if (activationState == CoreWindowActivationState_Deactivated) {
        QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationInactive);
        return S_OK;
    }

    QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationActive);

    // Activate topWindow
    if (!d->visibleWindows.isEmpty()) {
        Qt::FocusReason focusReason = activationState == CoreWindowActivationState_PointerActivated
                ? Qt::MouseFocusReason : Qt::ActiveWindowFocusReason;
        QWindowSystemInterface::handleWindowActivated(topWindow(), focusReason);
    }
    return S_OK;
}

HRESULT QWinRTScreen::onClosed(ICoreWindow *, ICoreWindowEventArgs *)
{
    qCDebug(lcQpaWindows) << __FUNCTION__;

    foreach (QWindow *w, QGuiApplication::topLevelWindows())
        QWindowSystemInterface::handleCloseEvent(w);
    return S_OK;
}

HRESULT QWinRTScreen::onVisibilityChanged(ICoreWindow *, IVisibilityChangedEventArgs *args)
{
    Q_D(QWinRTScreen);
    boolean visible;
    HRESULT hr = args ? args->get_Visible(&visible) : d->coreWindow->get_Visible(&visible);
    RETURN_OK_IF_FAILED("Failed to get visibility.");
    qCDebug(lcQpaWindows) << __FUNCTION__ << visible;
    QWindowSystemInterface::handleApplicationStateChanged(visible ? Qt::ApplicationActive : Qt::ApplicationHidden);
    if (visible) {
        handleExpose();
        onWindowSizeChanged(nullptr, nullptr);
    }
    return S_OK;
}

HRESULT QWinRTScreen::onOrientationChanged(IDisplayInformation *, IInspectable *)
{
    Q_D(QWinRTScreen);
    qCDebug(lcQpaWindows) << __FUNCTION__;
    DisplayOrientations displayOrientation;
    HRESULT hr = d->displayInformation->get_CurrentOrientation(&displayOrientation);
    RETURN_OK_IF_FAILED("Failed to get current orientations.");

    Qt::ScreenOrientation newOrientation = static_cast<Qt::ScreenOrientation>(static_cast<int>(qtOrientationsFromNative(displayOrientation)));
    if (d->orientation != newOrientation) {
        d->orientation = newOrientation;
        qCDebug(lcQpaWindows) << "  New orientation:" << newOrientation;
        onWindowSizeChanged(nullptr, nullptr);
        QWindowSystemInterface::handleScreenOrientationChange(screen(), d->orientation);
        handleExpose(); // Clean broken frames caused by race between Qt and ANGLE
    }
    return S_OK;
}

HRESULT QWinRTScreen::onDpiChanged(IDisplayInformation *, IInspectable *)
{
    Q_D(QWinRTScreen);

    HRESULT hr;
    ResolutionScale resolutionScale;
    hr = d->displayInformation->get_ResolutionScale(&resolutionScale);
    d->scaleFactor = qreal(resolutionScale) / 100;

    qCDebug(lcQpaWindows) << __FUNCTION__ << "Scale Factor:" << d->scaleFactor;

    RETURN_OK_IF_FAILED("Failed to get scale factor");

    FLOAT dpi;
    hr = d->displayInformation->get_LogicalDpi(&dpi);
    RETURN_OK_IF_FAILED("Failed to get logical DPI.");
    d->logicalDpi = dpi;

    hr = d->displayInformation->get_RawDpiX(&dpi);
    RETURN_OK_IF_FAILED("Failed to get x raw DPI.");
    d->physicalDpi.first = dpi ? dpi : 96.0;

    hr = d->displayInformation->get_RawDpiY(&dpi);
    RETURN_OK_IF_FAILED("Failed to get y raw DPI.");
    d->physicalDpi.second = dpi ? dpi : 96.0;
    qCDebug(lcQpaWindows) << __FUNCTION__ << "Logical DPI:" << d->logicalDpi
                          << "Physical DPI:" << d->physicalDpi;

    return S_OK;
}

HRESULT QWinRTScreen::onRedirectReleased(ICorePointerRedirector *, IPointerEventArgs *args)
{
    // When dragging ends with a non-mouse input device then onRedirectRelease is invoked.
    // QTBUG-58781
    return onPointerUpdated(nullptr, args);
}

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)
HRESULT QWinRTScreen::onWindowSizeChanged(IApplicationView *, IInspectable *)
#else
HRESULT QWinRTScreen::onWindowSizeChanged(ICoreWindow *, IWindowSizeChangedEventArgs *)
#endif // WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)
{
    Q_D(QWinRTScreen);

    HRESULT hr;
    Rect windowSize;

    hr = d->coreWindow->get_Bounds(&windowSize);
    RETURN_OK_IF_FAILED("Failed to get window bounds");
    d->logicalRect = QRectF(windowSize.X, windowSize.Y, windowSize.Width, windowSize.Height);

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)
    Rect visibleRect;
    hr = d->view2->get_VisibleBounds(&visibleRect);
    RETURN_OK_IF_FAILED("Failed to get window visible bounds");
    d->visibleRect = QRectF(visibleRect.X, visibleRect.Y, visibleRect.Width, visibleRect.Height);
#else
    d->visibleRect = QRectF(windowSize.X, windowSize.Y, windowSize.Width, windowSize.Height);
#endif // WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)

    qCDebug(lcQpaWindows) << __FUNCTION__ << d->logicalRect;
    QWindowSystemInterface::handleScreenGeometryChange(screen(), geometry(), availableGeometry());
    QPlatformScreen::resizeMaximizedWindows();
    handleExpose();
    return S_OK;
}

QT_END_NAMESPACE
