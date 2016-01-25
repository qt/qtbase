/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwinrtscreen.h"

#include "qwinrtbackingstore.h"
#include "qwinrtinputcontext.h"
#include "qwinrtcursor.h"
#include <private/qeventdispatcher_winrt_p.h>

#include <QtGui/QSurfaceFormat>
#include <QtGui/QGuiApplication>
#include <qpa/qwindowsysteminterface.h>
#include <QtCore/qt_windows.h>
#include <QtCore/qfunctions_winrt.h>

#include <functional>
#include <wrl.h>
#include <windows.system.h>
#include <Windows.Applicationmodel.h>
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
#ifdef Q_OS_WINPHONE
typedef ITypedEventHandler<StatusBar*, IInspectable*> StatusBarHandler;
#endif

QT_BEGIN_NAMESPACE

struct KeyInfo {
    KeyInfo()
        : virtualKey(0)
    {
    }

    KeyInfo(const QString &text, quint32 virtualKey)
        : text(text)
        , virtualKey(virtualKey)
    {
    }

    KeyInfo(quint32 virtualKey)
        : virtualKey(virtualKey)
    {
    }

    QString text;
    quint32 virtualKey;
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
#ifdef Q_OS_WINPHONE
typedef HRESULT (__stdcall IStatusBar::*StatusBarCallbackRemover)(EventRegistrationToken);
uint qHash(StatusBarCallbackRemover key) { void *ptr = *(void **)(&key); return qHash(ptr); }
#endif

class QWinRTScreenPrivate
{
public:
    QTouchDevice *touchDevice;
    ComPtr<ICoreWindow> coreWindow;
    ComPtr<Xaml::IDependencyObject> canvas;
    ComPtr<IApplicationView> view;
    ComPtr<IDisplayInformation> displayInformation;
#ifdef Q_OS_WINPHONE
    ComPtr<IStatusBar> statusBar;
#endif

    QScopedPointer<QWinRTCursor> cursor;
    QHash<quint32, QWindowSystemInterface::TouchPoint> touchPoints;
    QSizeF logicalSize;
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
#ifdef Q_OS_WINPHONE
    QHash<StatusBarCallbackRemover, EventRegistrationToken> statusBarTokens;
#endif
};

// To be called from the XAML thread
QWinRTScreen::QWinRTScreen()
    : d_ptr(new QWinRTScreenPrivate)
{
    Q_D(QWinRTScreen);
    d->orientation = Qt::PrimaryOrientation;
    d->touchDevice = Q_NULLPTR;

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
    hr = d->coreWindow->Activate();
    Q_ASSERT_SUCCEEDED(hr);

    Rect rect;
    hr = d->coreWindow->get_Bounds(&rect);
    Q_ASSERT_SUCCEEDED(hr);
    d->logicalSize = QSizeF(rect.Width, rect.Height);

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
    onDpiChanged(Q_NULLPTR, Q_NULLPTR);
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
    hr = frameworkElement->put_Width(d->logicalSize.width());
    Q_ASSERT_SUCCEEDED(hr);
    hr = frameworkElement->put_Height(d->logicalSize.height());
    Q_ASSERT_SUCCEEDED(hr);
    ComPtr<Xaml::IUIElement> uiElement;
    hr = canvas.As(&uiElement);
    Q_ASSERT_SUCCEEDED(hr);
    hr = window->put_Content(uiElement.Get());
    Q_ASSERT_SUCCEEDED(hr);
    hr = canvas.As(&d->canvas);
    Q_ASSERT_SUCCEEDED(hr);

    d->cursor.reset(new QWinRTCursor);

#ifdef Q_OS_WINPHONE
    ComPtr<IStatusBarStatics> statusBarStatics;
    hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_UI_ViewManagement_StatusBar).Get(),
                                IID_PPV_ARGS(&statusBarStatics));
    Q_ASSERT_SUCCEEDED(hr);
    hr = statusBarStatics->GetForCurrentView(&d->statusBar);
    Q_ASSERT_SUCCEEDED(hr);
#endif // Q_OS_WINPHONE
}

QWinRTScreen::~QWinRTScreen()
{
    Q_D(QWinRTScreen);

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
#ifdef Q_OS_WINPHONE
        for (QHash<StatusBarCallbackRemover, EventRegistrationToken>::const_iterator i = d->statusBarTokens.begin(); i != d->statusBarTokens.end(); ++i) {
            hr = (d->statusBar.Get()->*i.key())(i.value());
            Q_ASSERT_SUCCEEDED(hr);
        }
#endif //Q_OS_WINPHONE
        return hr;
    });
    RETURN_VOID_IF_FAILED("Failed to unregister screen event callbacks");
}

QRect QWinRTScreen::geometry() const
{
    Q_D(const QWinRTScreen);
    return QRect(QPoint(), (d->logicalSize * d->scaleFactor).toSize());
}

#ifdef Q_OS_WINPHONE
QRect QWinRTScreen::availableGeometry() const
{
    Q_D(const QWinRTScreen);
    QRect statusBar;
    QEventDispatcherWinRT::runOnXamlThread([d, &statusBar]() {
        HRESULT hr;
        Rect rect;
        hr = d->statusBar->get_OccludedRect(&rect);
        Q_ASSERT_SUCCEEDED(hr);
        statusBar.setRect(qRound(rect.X * d->scaleFactor),
                          qRound(rect.Y * d->scaleFactor),
                          qRound(rect.Width * d->scaleFactor),
                          qRound(rect.Height * d->scaleFactor));
        return S_OK;
    });

    return geometry().adjusted(
                d->orientation == Qt::LandscapeOrientation ? statusBar.width() : 0,
                d->orientation == Qt::PortraitOrientation ? statusBar.height() : 0,
                d->orientation == Qt::InvertedLandscapeOrientation ? -statusBar.width() : 0,
                0);
}
#endif //Q_OS_WINPHONE

int QWinRTScreen::depth() const
{
    return 32;
}

QImage::Format QWinRTScreen::format() const
{
    return QImage::Format_ARGB32_Premultiplied;
}

QSizeF QWinRTScreen::physicalSize() const
{
    Q_D(const QWinRTScreen);
    return QSizeF(d->logicalSize.width() * d->scaleFactor / d->physicalDpi.first * qreal(25.4),
                  d->logicalSize.height() * d->scaleFactor / d->physicalDpi.second * qreal(25.4));
}

QDpi QWinRTScreen::logicalDpi() const
{
    Q_D(const QWinRTScreen);
    return QDpi(d->logicalDpi, d->logicalDpi);
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
    d->coreWindow->GetAsyncKeyState(VirtualKey_Shift, &mod);
    if (mod == CoreVirtualKeyStates_Down)
        mods |= Qt::ShiftModifier;
    d->coreWindow->GetAsyncKeyState(VirtualKey_Menu, &mod);
    if (mod == CoreVirtualKeyStates_Down)
        mods |= Qt::AltModifier;
    d->coreWindow->GetAsyncKeyState(VirtualKey_Control, &mod);
    if (mod == CoreVirtualKeyStates_Down)
        mods |= Qt::ControlModifier;
    d->coreWindow->GetAsyncKeyState(VirtualKey_LeftWindows, &mod);
    if (mod == CoreVirtualKeyStates_Down) {
        mods |= Qt::MetaModifier;
    } else {
        d->coreWindow->GetAsyncKeyState(VirtualKey_RightWindows, &mod);
        if (mod == CoreVirtualKeyStates_Down)
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

#ifdef Q_OS_WINPHONE
void QWinRTScreen::setStatusBarVisibility(bool visible, QWindow *window)
{
    Q_D(QWinRTScreen);
    const Qt::WindowFlags windowType = window->flags() & Qt::WindowType_Mask;
    if (!window || (windowType != Qt::Window && windowType != Qt::Dialog))
        return;

    QEventDispatcherWinRT::runOnXamlThread([d, visible]() {
        HRESULT hr;
        ComPtr<IAsyncAction> op;
        if (visible)
            hr = d->statusBar->ShowAsync(&op);
        else
            hr = d->statusBar->HideAsync(&op);
        Q_ASSERT_SUCCEEDED(hr);
        return S_OK;
    });
}
#endif //Q_OS_WINPHONE

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
#ifndef Q_OS_WINPHONE
    hr = d->coreWindow->add_SizeChanged(Callback<SizeChangedHandler>(this, &QWinRTScreen::onSizeChanged).Get(), &d->windowTokens[&ICoreWindow::remove_SizeChanged]);
    Q_ASSERT_SUCCEEDED(hr);
#else
    hr = d->statusBar->add_Showing(Callback<StatusBarHandler>(this, &QWinRTScreen::onStatusBarShowing).Get(), &d->statusBarTokens[&IStatusBar::remove_Showing]);
    Q_ASSERT_SUCCEEDED(hr);
    hr = d->statusBar->add_Hiding(Callback<StatusBarHandler>(this, &QWinRTScreen::onStatusBarHiding).Get(), &d->statusBarTokens[&IStatusBar::remove_Hiding]);
    Q_ASSERT_SUCCEEDED(hr);
#endif
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
    onOrientationChanged(Q_NULLPTR, Q_NULLPTR);
    onVisibilityChanged(nullptr, nullptr);
}

QWindow *QWinRTScreen::topWindow() const
{
    Q_D(const QWinRTScreen);
    return d->visibleWindows.isEmpty() ? 0 : d->visibleWindows.first();
}

void QWinRTScreen::addWindow(QWindow *window)
{
    Q_D(QWinRTScreen);
    if (window == topWindow())
        return;

#ifdef Q_OS_WINPHONE
    if (window->visibility() != QWindow::Maximized && window->visibility() != QWindow::Windowed)
        setStatusBarVisibility(false, window);
#endif

    d->visibleWindows.prepend(window);
    QWindowSystemInterface::handleWindowActivated(window, Qt::OtherFocusReason);
    handleExpose();
    QWindowSystemInterface::flushWindowSystemEvents();
}

void QWinRTScreen::removeWindow(QWindow *window)
{
    Q_D(QWinRTScreen);

#ifdef Q_OS_WINPHONE
    if (window->visibility() == QWindow::Minimized)
        setStatusBarVisibility(false, window);
#endif

    const bool wasTopWindow = window == topWindow();
    if (!d->visibleWindows.removeAll(window))
        return;
    if (wasTopWindow)
        QWindowSystemInterface::handleWindowActivated(window, Qt::OtherFocusReason);
    handleExpose();
    QWindowSystemInterface::flushWindowSystemEvents();
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
    d->visibleWindows.removeAll(window);
    d->visibleWindows.append(window);
    if (wasTopWindow)
        QWindowSystemInterface::handleWindowActivated(window, Qt::OtherFocusReason);
    handleExpose();
}

void QWinRTScreen::updateWindowTitle()
{
    Q_D(QWinRTScreen);

    QWindow *window = topWindow();
    if (!window)
        return;

    const QString title = window->title();
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
    // Defer character key presses to onCharacterReceived
    if (key == Qt::Key_unknown || (key >= Qt::Key_Space && key <= Qt::Key_ydiaeresis)) {
        d->activeKeys.insert(key, KeyInfo(virtualKey));
        return S_OK;
    }

    QWindowSystemInterface::handleExtendedKeyEvent(
                topWindow(),
                QEvent::KeyPress,
                key,
                keyboardModifiers(),
                !status.ScanCode ? -1 : status.ScanCode,
                virtualKey,
                0,
                QString(),
                status.RepeatCount > 1,
                !status.RepeatCount ? 1 : status.RepeatCount,
                false);
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
                status.RepeatCount > 1,
                !status.RepeatCount ? 1 : status.RepeatCount,
                false);
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
    const quint32 virtualKey = d->activeKeys.value(key).virtualKey;
    QWindowSystemInterface::handleExtendedKeyEvent(
                topWindow(),
                QEvent::KeyPress,
                key,
                modifiers,
                !status.ScanCode ? -1 : status.ScanCode,
                virtualKey,
                0,
                text,
                status.RepeatCount > 1,
                !status.RepeatCount ? 1 : status.RepeatCount,
                false);
    d->activeKeys.insert(key, KeyInfo(text, virtualKey));
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

        QWindowSystemInterface::handleEnterEvent(topWindow(), pos, pos);
    }
    return S_OK;
}

HRESULT QWinRTScreen::onPointerExited(ICoreWindow *, IPointerEventArgs *)
{
    QWindowSystemInterface::handleLeaveEvent(0);
    return S_OK;
}

HRESULT QWinRTScreen::onPointerUpdated(ICoreWindow *, IPointerEventArgs *args)
{
    Q_D(QWinRTScreen);
    ComPtr<IPointerPoint> pointerPoint;
    if (FAILED(args->get_CurrentPoint(&pointerPoint)))
        return E_INVALIDARG;

    // Common traits - point, modifiers, properties
    Point point;
    pointerPoint->get_Position(&point);
    QPointF pos(point.X * d->scaleFactor, point.Y * d->scaleFactor);
    QPointF localPos = pos;
    if (topWindow()) {
        const QPointF globalPosDelta = pos - pos.toPoint();
        localPos = topWindow()->mapFromGlobal(pos.toPoint()) + globalPosDelta;
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
            QWindowSystemInterface::handleWheelEvent(topWindow(), localPos, pos, QPoint(), angleDelta, mods);
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

        QWindowSystemInterface::handleMouseEvent(topWindow(), localPos, pos, buttons, mods);

        break;
    }
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

        QHash<quint32, QWindowSystemInterface::TouchPoint>::iterator it = d->touchPoints.find(id);
        if (it != d->touchPoints.end()) {
            boolean isPressed;
#ifndef Q_OS_WINPHONE
            pointerPoint->get_IsInContact(&isPressed);
#else
            properties->get_IsLeftButtonPressed(&isPressed); // IsInContact not reliable on phone
#endif
            it.value().state = isPressed ? Qt::TouchPointMoved : Qt::TouchPointReleased;
        } else {
            it = d->touchPoints.insert(id, QWindowSystemInterface::TouchPoint());
            it.value().state = Qt::TouchPointPressed;
            it.value().id = id;
        }
        it.value().area = QRectF(area.X * d->scaleFactor, area.Y * d->scaleFactor,
                                 area.Width * d->scaleFactor, area.Height * d->scaleFactor);
        it.value().normalPosition = QPointF(point.X/d->logicalSize.width(), point.Y/d->logicalSize.height());
        it.value().pressure = pressure;

        QWindowSystemInterface::handleTouchEvent(topWindow(), d->touchDevice, d->touchPoints.values(), mods);

        // Remove released points, station others
        for (QHash<quint32, QWindowSystemInterface::TouchPoint>::iterator i = d->touchPoints.begin(); i != d->touchPoints.end();) {
            if (i.value().state == Qt::TouchPointReleased)
                i = d->touchPoints.erase(i);
            else
                (i++).value().state = Qt::TouchPointStationary;
        }

        break;
    }
    case PointerDeviceType_Pen: {
        quint32 id;
        pointerPoint->get_PointerId(&id);

        boolean isPressed;
        pointerPoint->get_IsInContact(&isPressed);

        boolean isEraser;
        properties->get_IsEraser(&isEraser);
        int pointerType = isEraser ? 3 : 1;

        float pressure;
        properties->get_Pressure(&pressure);

        float xTilt;
        properties->get_XTilt(&xTilt);

        float yTilt;
        properties->get_YTilt(&yTilt);

        float rotation;
        properties->get_Twist(&rotation);

        QWindowSystemInterface::handleTabletEvent(topWindow(), isPressed, pos, pos, 0,
                                                  pointerType, pressure, xTilt, yTilt,
                                                  0, rotation, 0, id, mods);

        break;
    }
    }

    return S_OK;
}

HRESULT QWinRTScreen::onSizeChanged(ICoreWindow *, IWindowSizeChangedEventArgs *)
{
    Q_D(QWinRTScreen);

    Rect size;
    HRESULT hr;
    hr = d->coreWindow->get_Bounds(&size);
    RETURN_OK_IF_FAILED("Failed to get window bounds");
    d->logicalSize = QSizeF(size.Width, size.Height);
    QWindowSystemInterface::handleScreenGeometryChange(screen(), geometry(), availableGeometry());
    QPlatformScreen::resizeMaximizedWindows();
    handleExpose();
    return S_OK;
}

HRESULT QWinRTScreen::onActivated(ICoreWindow *, IWindowActivatedEventArgs *args)
{
    Q_D(QWinRTScreen);

    CoreWindowActivationState activationState;
    args->get_WindowActivationState(&activationState);
    if (activationState == CoreWindowActivationState_Deactivated) {
        QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationInactive);
        return S_OK;
    }

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
    QWindowSystemInterface::handleApplicationStateChanged(visible ? Qt::ApplicationActive : Qt::ApplicationHidden);
    if (visible)
        handleExpose();
    return S_OK;
}

HRESULT QWinRTScreen::onOrientationChanged(IDisplayInformation *, IInspectable *)
{
    Q_D(QWinRTScreen);

    DisplayOrientations displayOrientation;
    HRESULT hr = d->displayInformation->get_CurrentOrientation(&displayOrientation);
    RETURN_OK_IF_FAILED("Failed to get current orientations.");

    Qt::ScreenOrientation newOrientation = static_cast<Qt::ScreenOrientation>(static_cast<int>(qtOrientationsFromNative(displayOrientation)));
    if (d->orientation != newOrientation) {
        d->orientation = newOrientation;
#ifdef Q_OS_WINPHONE
        onSizeChanged(nullptr, nullptr);
#endif
        QWindowSystemInterface::handleScreenOrientationChange(screen(), d->orientation);
        handleExpose(); // Clean broken frames caused by race between Qt and ANGLE
    }
    return S_OK;
}

HRESULT QWinRTScreen::onDpiChanged(IDisplayInformation *, IInspectable *)
{
    Q_D(QWinRTScreen);

    HRESULT hr;
#ifdef Q_OS_WINPHONE
    ComPtr<IDisplayInformation2> displayInformation;
    hr = d->displayInformation.As(&displayInformation);
    RETURN_OK_IF_FAILED("Failed to cast display information.");
    hr = displayInformation->get_RawPixelsPerViewPixel(&d->scaleFactor);
#else
    ResolutionScale resolutionScale;
    hr = d->displayInformation->get_ResolutionScale(&resolutionScale);
    d->scaleFactor = qreal(resolutionScale) / 100;
#endif
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

    return S_OK;
}

#ifdef Q_OS_WINPHONE
HRESULT QWinRTScreen::onStatusBarShowing(IStatusBar *, IInspectable *)
{
    onSizeChanged(nullptr, nullptr);
    return S_OK;
}

HRESULT QWinRTScreen::onStatusBarHiding(IStatusBar *, IInspectable *)
{
    onSizeChanged(nullptr, nullptr);
    return S_OK;
}
#endif //Q_OS_WINPHONE

QT_END_NAMESPACE
