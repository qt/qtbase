/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwinrtscreen.h"

#include "qwinrtbackingstore.h"
#include "qwinrtinputcontext.h"
#include "qwinrtcursor.h"
#include "qwinrteglcontext.h"

#include <QtGui/QSurfaceFormat>
#include <QtGui/QGuiApplication>
#include <QtPlatformSupport/private/qeglconvenience_p.h>
#include <qpa/qwindowsysteminterface.h>
#include <QtCore/qt_windows.h>

#include <wrl.h>
#include <windows.system.h>
#include <windows.devices.input.h>
#include <windows.ui.h>
#include <windows.ui.core.h>
#include <windows.ui.input.h>
#include <windows.ui.viewmanagement.h>
#include <windows.graphics.display.h>
#include <windows.foundation.h>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::System;
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
typedef ITypedEventHandler<CoreWindow*, AutomationProviderRequestedEventArgs*> AutomationProviderRequestedHandler;

QT_BEGIN_NAMESPACE

static inline Qt::ScreenOrientation qOrientationFromNative(DisplayOrientations orientation)
{
    switch (orientation) {
    default:
    case DisplayOrientations_None:
        return Qt::PrimaryOrientation;
    case DisplayOrientations_Landscape:
        return Qt::LandscapeOrientation;
    case DisplayOrientations_LandscapeFlipped:
        return Qt::InvertedLandscapeOrientation;
    case DisplayOrientations_Portrait:
        return Qt::PortraitOrientation;
    case DisplayOrientations_PortraitFlipped:
        return Qt::InvertedPortraitOrientation;
    }
}

static inline Qt::KeyboardModifiers qKeyModifiers(ICoreWindow *window)
{
    Qt::KeyboardModifiers mods;
    CoreVirtualKeyStates mod;
    window->GetAsyncKeyState(VirtualKey_Shift, &mod);
    if (mod == CoreVirtualKeyStates_Down)
        mods |= Qt::ShiftModifier;
    window->GetAsyncKeyState(VirtualKey_Menu, &mod);
    if (mod == CoreVirtualKeyStates_Down)
        mods |= Qt::AltModifier;
    window->GetAsyncKeyState(VirtualKey_Control, &mod);
    if (mod == CoreVirtualKeyStates_Down)
        mods |= Qt::ControlModifier;
    window->GetAsyncKeyState(VirtualKey_LeftWindows, &mod);
    if (mod == CoreVirtualKeyStates_Down) {
        mods |= Qt::MetaModifier;
    } else {
        window->GetAsyncKeyState(VirtualKey_RightWindows, &mod);
        if (mod == CoreVirtualKeyStates_Down)
            mods |= Qt::MetaModifier;
    }
    return mods;
}

// Return Qt meta key from VirtualKey (discard character keys)
static inline Qt::Key qMetaKeyFromVirtual(VirtualKey key)
{
    switch (key) {

    default:
        return Qt::Key_unknown;

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
    case VirtualKey_Back:
        return Qt::Key_Back;
    case VirtualKey_Clear:
        return Qt::Key_Clear;
    case VirtualKey_Application:
        return Qt::Key_ApplicationLeft;
    case VirtualKey_Sleep:
        return Qt::Key_Sleep;
    case VirtualKey_Pause:
        return Qt::Key_Pause;
    case VirtualKey_Space:
        return Qt::Key_Space;
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

    /* Character keys - pass through.
    case VirtualKey_Enter:
    case VirtualKey_Tab:
    case VirtualKey_Number0:
    case VirtualKey_Number1:
    case VirtualKey_Number2:
    case VirtualKey_Number3:
    case VirtualKey_Number4:
    case VirtualKey_Number5:
    case VirtualKey_Number6:
    case VirtualKey_Number7:
    case VirtualKey_Number8:
    case VirtualKey_Number9:
    case VirtualKey_A:
    case VirtualKey_B:
    case VirtualKey_C:
    case VirtualKey_D:
    case VirtualKey_E:
    case VirtualKey_F:
    case VirtualKey_G:
    case VirtualKey_H:
    case VirtualKey_I:
    case VirtualKey_J:
    case VirtualKey_K:
    case VirtualKey_L:
    case VirtualKey_M:
    case VirtualKey_N:
    case VirtualKey_O:
    case VirtualKey_P:
    case VirtualKey_Q:
    case VirtualKey_R:
    case VirtualKey_S:
    case VirtualKey_T:
    case VirtualKey_U:
    case VirtualKey_V:
    case VirtualKey_W:
    case VirtualKey_X:
    case VirtualKey_Y:
    case VirtualKey_Z:
    case VirtualKey_Multiply:
    case VirtualKey_Add:
    case VirtualKey_Separator:
    case VirtualKey_Subtract:
    case VirtualKey_Decimal:
    case VirtualKey_Divide:*/

    /* NumberPad keys. No special Alt handling is needed, as WinRT doesn't send events if Alt is pressed.
    case VirtualKey_NumberPad0:
    case VirtualKey_NumberPad1:
    case VirtualKey_NumberPad2:
    case VirtualKey_NumberPad3:
    case VirtualKey_NumberPad4:
    case VirtualKey_NumberPad5:
    case VirtualKey_NumberPad6:
    case VirtualKey_NumberPad7:
    case VirtualKey_NumberPad8:
    case VirtualKey_NumberPad9:*/

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

// Map Qt keys from char
static inline Qt::Key qKeyFromChar(quint32 code, Qt::KeyboardModifiers mods = Qt::NoModifier)
{
    switch (code) {
    case 0x1:
    case 'a':
    case 'A':
        return Qt::Key_A;
    case 0x2:
    case 'b':
    case 'B':
        return Qt::Key_B;
    case 0x3:
    case 'c':
    case 'C':
        return Qt::Key_C;
    case 0x4:
    case 'd':
    case 'D':
        return Qt::Key_D;
    case 0x5:
    case 'e':
    case 'E':
        return Qt::Key_E;
    case 0x6:
    case 'f':
    case 'F':
        return Qt::Key_F;
    case 0x7:
    case 'g':
    case 'G':
        return Qt::Key_G;
    case 0x8:
    //case '\b':
        return mods & Qt::ControlModifier ? Qt::Key_H : Qt::Key_Backspace;
    case 'h':
    case 'H':
        return Qt::Key_H;
    case 0x9:
    //case '\t':
        return mods & Qt::ControlModifier ? Qt::Key_I : Qt::Key_Tab;
    case 'i':
    case 'I':
        return Qt::Key_I;
    case 0xa:
    //case '\n':
        return mods & Qt::ControlModifier ? Qt::Key_J : Qt::Key_Enter;
    case 'j':
    case 'J':
        return Qt::Key_J;
    case 0xb:
    case 'k':
    case 'K':
        return Qt::Key_K;
    case 0xc:
    case 'l':
    case 'L':
        return Qt::Key_L;
    case 0xd:
    case 'm':
    case 'M':
        return Qt::Key_M;
    case 0xe:
    case 'n':
    case 'N':
        return Qt::Key_N;
    case 0xf:
    case 'o':
    case 'O':
        return Qt::Key_O;
    case 0x10:
    case 'p':
    case 'P':
        return Qt::Key_P;
    case 0x11:
    case 'q':
    case 'Q':
        return Qt::Key_Q;
    case 0x12:
    case 'r':
    case 'R':
        return Qt::Key_R;
    case 0x13:
    case 's':
    case 'S':
        return Qt::Key_S;
    case 0x14:
    case 't':
    case 'T':
        return Qt::Key_T;
    case 0x15:
    case 'u':
    case 'U':
        return Qt::Key_U;
    case 0x16:
    case 'v':
    case 'V':
        return Qt::Key_V;
    case 0x17:
    case 'w':
    case 'W':
        return Qt::Key_W;
    case 0x18:
    case 'x':
    case 'X':
        return Qt::Key_X;
    case 0x19:
    case 'y':
    case 'Y':
        return Qt::Key_Y;
    case 0x1A:
    case 'z':
    case 'Z':
        return Qt::Key_Z;
    }
    return Qt::Key_unknown;
}

QWinRTScreen::QWinRTScreen(ICoreWindow *window)
    : m_coreWindow(window)
    , m_depth(32)
    , m_format(QImage::Format_ARGB32_Premultiplied)
#ifdef Q_OS_WINPHONE
    , m_inputContext(new QWinRTInputContext(m_coreWindow))
#else
    , m_inputContext(Make<QWinRTInputContext>(m_coreWindow).Detach())
#endif
    , m_cursor(new QWinRTCursor(window))
    , m_orientation(Qt::PrimaryOrientation)
{
#ifdef Q_OS_WINPHONE // On phone, there can be only one touch device
    QTouchDevice *touchDevice = new QTouchDevice;
    touchDevice->setCapabilities(QTouchDevice::Position | QTouchDevice::Area | QTouchDevice::Pressure);
    touchDevice->setType(QTouchDevice::TouchScreen);
    touchDevice->setName(QStringLiteral("WinPhoneTouchScreen"));
    Pointer pointer = { Pointer::TouchScreen, touchDevice };
    m_pointers.insert(0, pointer);
    QWindowSystemInterface::registerTouchDevice(touchDevice);
#endif

    Rect rect;
    window->get_Bounds(&rect);
    m_geometry = QRect(0, 0, rect.Width, rect.Height);

    m_surfaceFormat.setAlphaBufferSize(0);
    m_surfaceFormat.setRedBufferSize(8);
    m_surfaceFormat.setGreenBufferSize(8);
    m_surfaceFormat.setBlueBufferSize(8);

    m_surfaceFormat.setRenderableType(QSurfaceFormat::OpenGLES);
    m_surfaceFormat.setSamples(1);
    m_surfaceFormat.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    m_surfaceFormat.setDepthBufferSize(24);
    m_surfaceFormat.setStencilBufferSize(8);

    m_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (m_eglDisplay == EGL_NO_DISPLAY)
        qFatal("Qt WinRT platform plugin: failed to initialize EGL display.");

    if (!eglInitialize(m_eglDisplay, NULL, NULL))
        qFatal("Qt WinRT platform plugin: failed to initialize EGL. This can happen if you haven't included the D3D compiler DLL in your application package.");

    // TODO: move this to Window
    m_eglSurface = eglCreateWindowSurface(m_eglDisplay, q_configFromGLFormat(m_eglDisplay, m_surfaceFormat), window, NULL);
    if (m_eglSurface == EGL_NO_SURFACE)
        qFatal("Could not create EGL surface, error 0x%X", eglGetError());

    // Event handlers mapped to QEvents
    m_coreWindow->add_KeyDown(Callback<KeyHandler>(this, &QWinRTScreen::onKey).Get(), &m_tokens[QEvent::KeyPress]);
    m_coreWindow->add_KeyUp(Callback<KeyHandler>(this, &QWinRTScreen::onKey).Get(), &m_tokens[QEvent::KeyRelease]);
    m_coreWindow->add_CharacterReceived(Callback<CharacterReceivedHandler>(this, &QWinRTScreen::onCharacterReceived).Get(), &m_tokens[QEvent::User]);
    m_coreWindow->add_PointerEntered(Callback<PointerHandler>(this, &QWinRTScreen::onPointerEntered).Get(), &m_tokens[QEvent::Enter]);
    m_coreWindow->add_PointerExited(Callback<PointerHandler>(this, &QWinRTScreen::onPointerExited).Get(), &m_tokens[QEvent::Leave]);
    m_coreWindow->add_PointerMoved(Callback<PointerHandler>(this, &QWinRTScreen::onPointerUpdated).Get(), &m_tokens[QEvent::MouseMove]);
    m_coreWindow->add_PointerPressed(Callback<PointerHandler>(this, &QWinRTScreen::onPointerUpdated).Get(), &m_tokens[QEvent::MouseButtonPress]);
    m_coreWindow->add_PointerReleased(Callback<PointerHandler>(this, &QWinRTScreen::onPointerUpdated).Get(), &m_tokens[QEvent::MouseButtonRelease]);
    m_coreWindow->add_PointerWheelChanged(Callback<PointerHandler>(this, &QWinRTScreen::onPointerUpdated).Get(), &m_tokens[QEvent::Wheel]);
    m_coreWindow->add_SizeChanged(Callback<SizeChangedHandler>(this, &QWinRTScreen::onSizeChanged).Get(), &m_tokens[QEvent::Resize]);

    // Window event handlers
    m_coreWindow->add_Activated(Callback<ActivatedHandler>(this, &QWinRTScreen::onActivated).Get(), &m_tokens[QEvent::WindowActivate]);
    m_coreWindow->add_Closed(Callback<ClosedHandler>(this, &QWinRTScreen::onClosed).Get(), &m_tokens[QEvent::WindowDeactivate]);
    m_coreWindow->add_VisibilityChanged(Callback<VisibilityChangedHandler>(this, &QWinRTScreen::onVisibilityChanged).Get(), &m_tokens[QEvent::Show]);
    m_coreWindow->add_AutomationProviderRequested(Callback<AutomationProviderRequestedHandler>(this, &QWinRTScreen::onAutomationProviderRequested).Get(), &m_tokens[QEvent::InputMethodQuery]);

    // Orientation handling
    if (SUCCEEDED(GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Graphics_Display_DisplayProperties).Get(),
                                       &m_displayProperties))) {
        // Set native orientation
        DisplayOrientations displayOrientation;
        m_displayProperties->get_NativeOrientation(&displayOrientation);
        m_nativeOrientation = qOrientationFromNative(displayOrientation);

        // Set initial orientation
        onOrientationChanged(0);

        m_displayProperties->add_OrientationChanged(Callback<IDisplayPropertiesEventHandler>(this, &QWinRTScreen::onOrientationChanged).Get(),
                                                    &m_tokens[QEvent::OrientationChange]);
    }

#ifndef Q_OS_WINPHONE
    GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_UI_ViewManagement_ApplicationView).Get(),
                         &m_applicationView);
#endif
}

QRect QWinRTScreen::geometry() const
{
    return m_geometry;
}

int QWinRTScreen::depth() const
{
    return m_depth;
}

QImage::Format QWinRTScreen::format() const
{
    return m_format;
}

QSurfaceFormat QWinRTScreen::surfaceFormat() const
{
    return m_surfaceFormat;
}

QWinRTInputContext *QWinRTScreen::inputContext() const
{
    return m_inputContext;
}

QPlatformCursor *QWinRTScreen::cursor() const
{
    return m_cursor;
}

Qt::ScreenOrientation QWinRTScreen::nativeOrientation() const
{
    return m_nativeOrientation;
}

Qt::ScreenOrientation QWinRTScreen::orientation() const
{
    return m_orientation;
}

ICoreWindow *QWinRTScreen::coreWindow() const
{
    return m_coreWindow;
}

EGLDisplay QWinRTScreen::eglDisplay() const
{
    return m_eglDisplay;
}

EGLSurface QWinRTScreen::eglSurface() const
{
    return m_eglSurface;
}

QWindow *QWinRTScreen::topWindow() const
{
    return m_visibleWindows.isEmpty() ? 0 : m_visibleWindows.first();
}

void QWinRTScreen::addWindow(QWindow *window)
{
    if (window == topWindow())
        return;
    m_visibleWindows.prepend(window);
    QWindowSystemInterface::handleWindowActivated(window, Qt::OtherFocusReason);
    handleExpose();
}

void QWinRTScreen::removeWindow(QWindow *window)
{
    const bool wasTopWindow = window == topWindow();
    if (!m_visibleWindows.removeAll(window))
        return;
    if (wasTopWindow)
        QWindowSystemInterface::handleWindowActivated(window, Qt::OtherFocusReason);
    handleExpose();
}

void QWinRTScreen::raise(QWindow *window)
{
    m_visibleWindows.removeAll(window);
    addWindow(window);
}

void QWinRTScreen::lower(QWindow *window)
{
    const bool wasTopWindow = window == topWindow();
    if (wasTopWindow && m_visibleWindows.size() == 1)
        return;
    m_visibleWindows.removeAll(window);
    m_visibleWindows.append(window);
    if (wasTopWindow)
        QWindowSystemInterface::handleWindowActivated(window, Qt::OtherFocusReason);
    handleExpose();
}

void QWinRTScreen::handleExpose()
{
    if (m_visibleWindows.isEmpty())
        return;
    QList<QWindow *>::const_iterator it = m_visibleWindows.constBegin();
    QWindowSystemInterface::handleExposeEvent(*it, m_geometry);
    while (++it != m_visibleWindows.constEnd())
        QWindowSystemInterface::handleExposeEvent(*it, QRegion());
    QWindowSystemInterface::flushWindowSystemEvents();
}

HRESULT QWinRTScreen::onKey(ABI::Windows::UI::Core::ICoreWindow *window, ABI::Windows::UI::Core::IKeyEventArgs *args)
{
    Q_UNUSED(window);

    // Windows Phone documentation claims this will throw, but doesn't seem to
    CorePhysicalKeyStatus keyStatus;
    args->get_KeyStatus(&keyStatus);

    VirtualKey virtualKey;
    args->get_VirtualKey(&virtualKey);

    // Filter meta keys
    Qt::Key key = qMetaKeyFromVirtual(virtualKey);

    // Get keyboard modifiers. This could alternatively be tracked by key presses, but
    // WinRT doesn't send key events for Alt unless Ctrl is also pressed.
    // If the key that caused this event is a modifier, it is not returned in the flags.
    Qt::KeyboardModifiers mods = qKeyModifiers(m_coreWindow);

    if (m_activeKeys.contains(keyStatus.ScanCode)) { // Handle tracked keys (release/repeat)
        QString text = keyStatus.IsKeyReleased ? m_activeKeys.take(keyStatus.ScanCode) : m_activeKeys.value(keyStatus.ScanCode);
        QWindowSystemInterface::handleKeyEvent(topWindow(), QEvent::KeyRelease, key, mods, text);

        if (!keyStatus.IsKeyReleased) // Repeating key
            QWindowSystemInterface::handleKeyEvent(topWindow(), QEvent::KeyPress, key, mods, text);

    } else if (keyStatus.IsKeyReleased) { // Unlikely, but possible if key is held before application is focused
        QWindowSystemInterface::handleKeyEvent(topWindow(), QEvent::KeyRelease, key, mods);

    } else { // Handle key presses
        if (key != Qt::Key_unknown) // Handle non-character key presses here, others in onCharacterReceived
            QWindowSystemInterface::handleKeyEvent(topWindow(), QEvent::KeyPress, key, mods);

        m_activeKeys.insert(keyStatus.ScanCode, QString());
    }

    return S_OK;
}

HRESULT QWinRTScreen::onCharacterReceived(ICoreWindow *window, ICharacterReceivedEventArgs *args)
{
    Q_UNUSED(window);

    quint32 keyCode;
    args->get_KeyCode(&keyCode);

    // Windows Phone documentation claims this will throw, but doesn't seem to
    CorePhysicalKeyStatus keyStatus;
    args->get_KeyStatus(&keyStatus);

    QString text = QChar(keyCode);

    Qt::KeyboardModifiers mods = qKeyModifiers(m_coreWindow);
    Qt::Key key = qKeyFromChar(keyCode, mods);

    QWindowSystemInterface::handleKeyEvent(topWindow(), QEvent::KeyPress, key, mods, text);

    // Note that we can receive a character without corresponding press/release events, such as
    // the case of an Alt-combo. In this case, we should send the release immediately.
    if (m_activeKeys.contains(keyStatus.ScanCode))
        m_activeKeys.insert(keyStatus.ScanCode, text);
    else
        QWindowSystemInterface::handleKeyEvent(topWindow(), QEvent::KeyRelease, key, mods, text);

    return S_OK;
}

HRESULT QWinRTScreen::onPointerEntered(ICoreWindow *window, IPointerEventArgs *args)
{
    Q_UNUSED(window);
    IPointerPoint *pointerPoint;
    if (SUCCEEDED(args->get_CurrentPoint(&pointerPoint))) {
        // Assumes full-screen window
        Point point;
        pointerPoint->get_Position(&point);
        QPoint pos(point.X, point.Y);

        QWindowSystemInterface::handleEnterEvent(topWindow(), pos, pos);
        pointerPoint->Release();
    }
    return S_OK;
}

HRESULT QWinRTScreen::onPointerExited(ICoreWindow *window, IPointerEventArgs *args)
{
    Q_UNUSED(window);
    Q_UNUSED(args);
    QWindowSystemInterface::handleLeaveEvent(0);
    return S_OK;
}

HRESULT QWinRTScreen::onPointerUpdated(ICoreWindow *window, IPointerEventArgs *args)
{
    Q_UNUSED(window);

    IPointerPoint *pointerPoint;
    if (FAILED(args->get_CurrentPoint(&pointerPoint)))
        return E_INVALIDARG;

    // Common traits - point, modifiers, properties
    Point point;
    pointerPoint->get_Position(&point);
    QPointF pos(point.X, point.Y);

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

    IPointerPointProperties *properties;
    if (FAILED(pointerPoint->get_Properties(&properties)))
        return E_INVALIDARG;

#ifdef Q_OS_WINPHONE
    quint32 pointerId = 0;
    Pointer pointer = m_pointers.value(pointerId);
#else
    Pointer pointer = { Pointer::Unknown, 0 };
    quint32 pointerId;
    pointerPoint->get_PointerId(&pointerId);
    if (m_pointers.contains(pointerId)) {
        pointer = m_pointers.value(pointerId);
    } else { // We have not yet enumerated this device. Do so now...
        IPointerDevice *device;
        if (SUCCEEDED(pointerPoint->get_PointerDevice(&device))) {
            PointerDeviceType type;
            device->get_PointerDeviceType(&type);
            switch (type) {
            case PointerDeviceType_Touch:
                pointer.type = Pointer::TouchScreen;
                pointer.device = new QTouchDevice;
                pointer.device->setName(QStringLiteral("WinRT TouchScreen ") + QString::number(pointerId));
                // TODO: We may want to probe the device usage flags for more accurate values for these next two
                pointer.device->setType(QTouchDevice::TouchScreen);
                pointer.device->setCapabilities(QTouchDevice::Position | QTouchDevice::Area | QTouchDevice::Pressure);
                QWindowSystemInterface::registerTouchDevice(pointer.device);
                break;

            case PointerDeviceType_Pen:
                pointer.type = Pointer::Tablet;
                break;

            case PointerDeviceType_Mouse:
                pointer.type = Pointer::Mouse;
                break;
            }

            m_pointers.insert(pointerId, pointer);
            device->Release();
        }
    }
#endif
    switch (pointer.type) {
    case Pointer::Mouse: {
        qint32 delta;
        properties->get_MouseWheelDelta(&delta);
        if (delta) {
            boolean isHorizontal;
            properties->get_IsHorizontalMouseWheel(&isHorizontal);
            QPoint angleDelta(isHorizontal ? delta : 0, isHorizontal ? 0 : delta);
            QWindowSystemInterface::handleWheelEvent(topWindow(), pos, pos, QPoint(), angleDelta, mods);
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

        QWindowSystemInterface::handleMouseEvent(topWindow(), pos, pos, buttons, mods);

        break;
    }
    case Pointer::TouchScreen: {
        quint32 id;
        pointerPoint->get_PointerId(&id);

        Rect area;
        properties->get_ContactRect(&area);

        float pressure;
        properties->get_Pressure(&pressure);

        QHash<quint32, QWindowSystemInterface::TouchPoint>::iterator it = m_touchPoints.find(id);
        if (it != m_touchPoints.end()) {
            boolean isPressed;
            pointerPoint->get_IsInContact(&isPressed);
            it.value().state = isPressed ? Qt::TouchPointMoved : Qt::TouchPointReleased;
        } else {
            it = m_touchPoints.insert(id, QWindowSystemInterface::TouchPoint());
            it.value().state = Qt::TouchPointPressed;
            it.value().id = id;
        }
        it.value().area = QRectF(area.X, area.Y, area.Width, area.Height);
        it.value().normalPosition = QPointF(pos.x()/m_geometry.width(), pos.y()/m_geometry.height());
        it.value().pressure = pressure;

        QWindowSystemInterface::handleTouchEvent(topWindow(), pointer.device, m_touchPoints.values(), mods);

        // Remove released points, station others
        for (QHash<quint32, QWindowSystemInterface::TouchPoint>::iterator i = m_touchPoints.begin(); i != m_touchPoints.end();) {
            if (i.value().state == Qt::TouchPointReleased)
                i = m_touchPoints.erase(i);
            else
                (i++).value().state = Qt::TouchPointStationary;
        }

        break;
    }
    case Pointer::Tablet: {
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

        QWindowSystemInterface::handleTabletEvent(topWindow(), isPressed, pos, pos, pointerId,
                                                  pointerType, pressure, xTilt, yTilt,
                                                  0, rotation, 0, id, mods);

        break;
    }
    }

    properties->Release();
    pointerPoint->Release();

    return S_OK;
}

HRESULT QWinRTScreen::onAutomationProviderRequested(ICoreWindow *, IAutomationProviderRequestedEventArgs *args)
{
#ifndef Q_OS_WINPHONE
    args->put_AutomationProvider(m_inputContext);
#endif
    return S_OK;
}

HRESULT QWinRTScreen::onSizeChanged(ICoreWindow *window, IWindowSizeChangedEventArgs *args)
{
    Q_UNUSED(window);

    Size size;
    if (FAILED(args->get_Size(&size))) {
        qWarning(Q_FUNC_INFO ": failed to get size");
        return S_OK;
    }

    // Regardless of state, all top-level windows are viewport-sized - this might change if
    // a more advanced compositor is written.
    m_geometry.setSize(QSize(size.Width, size.Height));
    QWindowSystemInterface::handleScreenGeometryChange(screen(), m_geometry);
    QWindowSystemInterface::handleScreenAvailableGeometryChange(screen(), m_geometry);
    QPlatformScreen::resizeMaximizedWindows();
    handleExpose();

    return S_OK;
}

HRESULT QWinRTScreen::onActivated(ICoreWindow *window, IWindowActivatedEventArgs *args)
{
    Q_UNUSED(window);

    CoreWindowActivationState activationState;
    args->get_WindowActivationState(&activationState);
    if (activationState == CoreWindowActivationState_Deactivated) {
        QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationInactive);
        return S_OK;
    }

    // Activate topWindow
    if (!m_visibleWindows.isEmpty()) {
        Qt::FocusReason focusReason = activationState == CoreWindowActivationState_PointerActivated
                ? Qt::MouseFocusReason : Qt::ActiveWindowFocusReason;
        QWindowSystemInterface::handleWindowActivated(topWindow(), focusReason);
    }
    return S_OK;
}

HRESULT QWinRTScreen::onClosed(ICoreWindow *window, ICoreWindowEventArgs *args)
{
    Q_UNUSED(window);
    Q_UNUSED(args);

    foreach (QWindow *w, QGuiApplication::topLevelWindows())
        QWindowSystemInterface::handleCloseEvent(w);
    return S_OK;
}

HRESULT QWinRTScreen::onVisibilityChanged(ICoreWindow *window, IVisibilityChangedEventArgs *args)
{
    Q_UNUSED(window);
    Q_UNUSED(args);

    boolean visible;
    args->get_Visible(&visible);
    QWindowSystemInterface::handleApplicationStateChanged(visible ? Qt::ApplicationActive : Qt::ApplicationHidden);
    return S_OK;
}

HRESULT QWinRTScreen::onOrientationChanged(IInspectable *)
{
    DisplayOrientations displayOrientation;
    m_displayProperties->get_CurrentOrientation(&displayOrientation);
    Qt::ScreenOrientation newOrientation = qOrientationFromNative(displayOrientation);
    if (m_orientation != newOrientation) {
        m_orientation = newOrientation;
        QWindowSystemInterface::handleScreenOrientationChange(screen(), m_orientation);
    }

    return S_OK;
}

QT_END_NAMESPACE
