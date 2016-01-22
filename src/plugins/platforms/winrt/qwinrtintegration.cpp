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

#include "qwinrtintegration.h"
#include "qwinrtwindow.h"
#include "qwinrteventdispatcher.h"
#include "qwinrtbackingstore.h"
#include "qwinrtscreen.h"
#include "qwinrtinputcontext.h"
#include "qwinrtservices.h"
#include "qwinrteglcontext.h"
#include "qwinrtfontdatabase.h"
#include "qwinrttheme.h"
#include "qwinrtclipboard.h"

#include <QtGui/QOffscreenSurface>
#include <QtGui/QOpenGLContext>
#include <QtGui/QSurface>

#include <QtPlatformSupport/private/qeglpbuffer_p.h>
#include <qpa/qwindowsysteminterface.h>
#include <qpa/qplatformwindow.h>
#include <qpa/qplatformoffscreensurface.h>

#include <qfunctions_winrt.h>

#include <functional>
#include <wrl.h>
#include <windows.ui.xaml.h>
#include <windows.applicationmodel.h>
#include <windows.applicationmodel.core.h>
#include <windows.ui.core.h>
#include <windows.ui.viewmanagement.h>
#include <windows.graphics.display.h>

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)
#  include <windows.phone.ui.input.h>
#  if _MSC_VER >= 1900
#    include <windows.foundation.metadata.h>
     using namespace ABI::Windows::Foundation::Metadata;
#  endif
#endif


using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::ApplicationModel;
using namespace ABI::Windows::ApplicationModel::Core;
using namespace ABI::Windows::UI;
using namespace ABI::Windows::UI::Core;
using namespace ABI::Windows::UI::ViewManagement;
using namespace ABI::Windows::Graphics::Display;
using namespace ABI::Windows::ApplicationModel::Core;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)
using namespace ABI::Windows::Phone::UI::Input;
#endif

typedef IEventHandler<IInspectable *> ResumeHandler;
typedef IEventHandler<SuspendingEventArgs *> SuspendHandler;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)
typedef IEventHandler<BackPressedEventArgs*> BackPressedHandler;
typedef IEventHandler<CameraEventArgs*> CameraButtonHandler;
#endif

QT_BEGIN_NAMESPACE

typedef HRESULT (__stdcall ICoreApplication::*CoreApplicationCallbackRemover)(EventRegistrationToken);
uint qHash(CoreApplicationCallbackRemover key) { void *ptr = *(void **)(&key); return qHash(ptr); }
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)
typedef HRESULT (__stdcall IHardwareButtonsStatics::*HardwareButtonsCallbackRemover)(EventRegistrationToken);
uint qHash(HardwareButtonsCallbackRemover key) { void *ptr = *(void **)(&key); return qHash(ptr); }
typedef HRESULT (__stdcall IHardwareButtonsStatics2::*HardwareButtons2CallbackRemover)(EventRegistrationToken);
uint qHash(HardwareButtons2CallbackRemover key) { void *ptr = *(void **)(&key); return qHash(ptr); }
#endif

class QWinRTIntegrationPrivate
{
public:
    QPlatformFontDatabase *fontDatabase;
    QPlatformServices *platformServices;
    QPlatformClipboard *clipboard;
    QWinRTScreen *mainScreen;
    QScopedPointer<QWinRTInputContext> inputContext;

    ComPtr<ICoreApplication> application;
    QHash<CoreApplicationCallbackRemover, EventRegistrationToken> applicationTokens;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)
    ComPtr<IHardwareButtonsStatics> hardwareButtons;
    QHash<HardwareButtonsCallbackRemover, EventRegistrationToken> buttonsTokens;
    ComPtr<IHardwareButtonsStatics2> cameraButtons;
    QHash<HardwareButtons2CallbackRemover, EventRegistrationToken> cameraTokens;
    boolean hasHardwareButtons;
    bool cameraHalfPressed : 1;
    bool cameraPressed : 1;
#endif
};

QWinRTIntegration::QWinRTIntegration() : d_ptr(new QWinRTIntegrationPrivate)
{
    Q_D(QWinRTIntegration);

    d->fontDatabase = new QWinRTFontDatabase;

    HRESULT hr;
    hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_ApplicationModel_Core_CoreApplication).Get(),
                                IID_PPV_ARGS(&d->application));
    Q_ASSERT_SUCCEEDED(hr);
    hr = d->application->add_Suspending(Callback<SuspendHandler>(this, &QWinRTIntegration::onSuspended).Get(),
                                        &d->applicationTokens[&ICoreApplication::remove_Suspending]);
    Q_ASSERT_SUCCEEDED(hr);
    hr = d->application->add_Resuming(Callback<ResumeHandler>(this, &QWinRTIntegration::onResume).Get(),
                                      &d->applicationTokens[&ICoreApplication::remove_Resuming]);
    Q_ASSERT_SUCCEEDED(hr);

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)
#if _MSC_VER >= 1900
    d->hasHardwareButtons = false;
    ComPtr<IApiInformationStatics> apiInformationStatics;
    hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Foundation_Metadata_ApiInformation).Get(),
                                IID_PPV_ARGS(&apiInformationStatics));

    if (SUCCEEDED(hr)) {
        const HStringReference valueRef(L"Windows.Phone.UI.Input.HardwareButtons");
        hr = apiInformationStatics->IsTypePresent(valueRef.Get(), &d->hasHardwareButtons);
    }
#else
    d->hasHardwareButtons = true;
#endif // _MSC_VER >= 1900

    if (d->hasHardwareButtons) {
        hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Phone_UI_Input_HardwareButtons).Get(),
                                    IID_PPV_ARGS(&d->hardwareButtons));
        Q_ASSERT_SUCCEEDED(hr);
        hr = d->hardwareButtons->add_BackPressed(Callback<BackPressedHandler>(this, &QWinRTIntegration::onBackButtonPressed).Get(),
                                                 &d->buttonsTokens[&IHardwareButtonsStatics::remove_BackPressed]);
        Q_ASSERT_SUCCEEDED(hr);

        hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Phone_UI_Input_HardwareButtons).Get(),
                                    IID_PPV_ARGS(&d->cameraButtons));
        Q_ASSERT_SUCCEEDED(hr);
        if (qEnvironmentVariableIntValue("QT_QPA_ENABLE_CAMERA_KEYS")) {
            hr = d->cameraButtons->add_CameraPressed(Callback<CameraButtonHandler>(this, &QWinRTIntegration::onCameraPressed).Get(),
                                                     &d->cameraTokens[&IHardwareButtonsStatics2::remove_CameraPressed]);
            Q_ASSERT_SUCCEEDED(hr);
            hr = d->cameraButtons->add_CameraHalfPressed(Callback<CameraButtonHandler>(this, &QWinRTIntegration::onCameraHalfPressed).Get(),
                                                         &d->cameraTokens[&IHardwareButtonsStatics2::remove_CameraHalfPressed]);
            Q_ASSERT_SUCCEEDED(hr);
            hr = d->cameraButtons->add_CameraReleased(Callback<CameraButtonHandler>(this, &QWinRTIntegration::onCameraReleased).Get(),
                                                      &d->cameraTokens[&IHardwareButtonsStatics2::remove_CameraReleased]);
            Q_ASSERT_SUCCEEDED(hr);
        }
        d->cameraPressed = false;
        d->cameraHalfPressed = false;
    }
#endif // WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)


    QEventDispatcherWinRT::runOnXamlThread([d]() {
        d->mainScreen = new QWinRTScreen;
        d->inputContext.reset(new QWinRTInputContext(d->mainScreen));
        return S_OK;
    });

    screenAdded(d->mainScreen);
    d->platformServices = new QWinRTServices;
    d->clipboard = new QWinRTClipboard;
}

QWinRTIntegration::~QWinRTIntegration()
{
    Q_D(QWinRTIntegration);
    HRESULT hr;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)
    if (d->hasHardwareButtons) {
        for (QHash<HardwareButtonsCallbackRemover, EventRegistrationToken>::const_iterator i = d->buttonsTokens.begin(); i != d->buttonsTokens.end(); ++i) {
            hr = (d->hardwareButtons.Get()->*i.key())(i.value());
            Q_ASSERT_SUCCEEDED(hr);
        }
        for (QHash<HardwareButtons2CallbackRemover, EventRegistrationToken>::const_iterator i = d->cameraTokens.begin(); i != d->cameraTokens.end(); ++i) {
            hr = (d->cameraButtons.Get()->*i.key())(i.value());
            Q_ASSERT_SUCCEEDED(hr);
        }
    }
#endif
    // Do not execute this on Windows Phone as the application is already
    // shutting down and trying to unregister suspending/resume handler will
    // cause exceptions and assert in debug mode
#ifndef Q_OS_WINPHONE
    for (QHash<CoreApplicationCallbackRemover, EventRegistrationToken>::const_iterator i = d->applicationTokens.begin(); i != d->applicationTokens.end(); ++i) {
        hr = (d->application.Get()->*i.key())(i.value());
        Q_ASSERT_SUCCEEDED(hr);
    }
#endif
    destroyScreen(d->mainScreen);
    Windows::Foundation::Uninitialize();
}

bool QWinRTIntegration::succeeded() const
{
    Q_D(const QWinRTIntegration);
    return d->mainScreen;
}

QAbstractEventDispatcher *QWinRTIntegration::createEventDispatcher() const
{
    return new QWinRTEventDispatcher;
}

void QWinRTIntegration::initialize()
{
    Q_D(const QWinRTIntegration);
    QEventDispatcherWinRT::runOnXamlThread([d]() {
        d->mainScreen->initialize();
        return S_OK;
    });
}

bool QWinRTIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps:
    case OpenGL:
    case ApplicationState:
    case NonFullScreenWindows:
    case MultipleWindows:
    case RasterGLSurface:
        return true;
    default:
        return QPlatformIntegration::hasCapability(cap);
    }
}

QVariant QWinRTIntegration::styleHint(StyleHint hint) const
{
    return QWinRTTheme::styleHint(hint);
}

QPlatformWindow *QWinRTIntegration::createPlatformWindow(QWindow *window) const
{
    return new QWinRTWindow(window);
}

QPlatformBackingStore *QWinRTIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new QWinRTBackingStore(window);
}

QPlatformOpenGLContext *QWinRTIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    return new QWinRTEGLContext(context);
}

QPlatformFontDatabase *QWinRTIntegration::fontDatabase() const
{
    Q_D(const QWinRTIntegration);
    return d->fontDatabase;
}

QPlatformInputContext *QWinRTIntegration::inputContext() const
{
    Q_D(const QWinRTIntegration);
    return d->inputContext.data();
}

QPlatformServices *QWinRTIntegration::services() const
{
    Q_D(const QWinRTIntegration);
    return d->platformServices;
}

QPlatformClipboard *QWinRTIntegration::clipboard() const
{
    Q_D(const QWinRTIntegration);
    return d->clipboard;
}

Qt::KeyboardModifiers QWinRTIntegration::queryKeyboardModifiers() const
{
    Q_D(const QWinRTIntegration);
    return d->mainScreen->keyboardModifiers();
}

QStringList QWinRTIntegration::themeNames() const
{
    return QStringList(QLatin1String("winrt"));
}

QPlatformTheme *QWinRTIntegration::createPlatformTheme(const QString &name) const
{
    if (name == QLatin1String("winrt"))
        return new QWinRTTheme();

    return 0;
}

// System-level integration points

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)
HRESULT QWinRTIntegration::onBackButtonPressed(IInspectable *, IBackPressedEventArgs *args)
{
    Q_D(QWinRTIntegration);
    QWindow *window = d->mainScreen->topWindow();
    QWindowSystemInterface::setSynchronousWindowSystemEvents(true);
    const bool pressed = QWindowSystemInterface::handleExtendedKeyEvent(window, QEvent::KeyPress, Qt::Key_Back, Qt::NoModifier,
                                                                        0, 0, 0, QString(), false, 1, false);
    const bool released = QWindowSystemInterface::handleExtendedKeyEvent(window, QEvent::KeyRelease, Qt::Key_Back, Qt::NoModifier,
                                                                         0, 0, 0, QString(), false, 1, false);
    QWindowSystemInterface::setSynchronousWindowSystemEvents(false);
    args->put_Handled(pressed || released);
    return S_OK;
}

HRESULT QWinRTIntegration::onCameraPressed(IInspectable *, ICameraEventArgs *)
{
    Q_D(QWinRTIntegration);
    QWindow *window = d->mainScreen->topWindow();
    QWindowSystemInterface::handleExtendedKeyEvent(window, QEvent::KeyPress, Qt::Key_Camera, Qt::NoModifier,
                                                   0, 0, 0, QString(), false, 1, false);
    d->cameraPressed = true;
    return S_OK;
}

HRESULT QWinRTIntegration::onCameraHalfPressed(IInspectable *, ICameraEventArgs *)
{
    Q_D(QWinRTIntegration);
    QWindow *window = d->mainScreen->topWindow();
    QWindowSystemInterface::handleExtendedKeyEvent(window, QEvent::KeyPress, Qt::Key_CameraFocus, Qt::NoModifier,
                                                   0, 0, 0, QString(), false, 1, false);
    d->cameraHalfPressed = true;
    return S_OK;
}

HRESULT QWinRTIntegration::onCameraReleased(IInspectable *, ICameraEventArgs *)
{
    Q_D(QWinRTIntegration);
    QWindow *window = d->mainScreen->topWindow();
    if (d->cameraHalfPressed)
        QWindowSystemInterface::handleExtendedKeyEvent(window, QEvent::KeyRelease, Qt::Key_CameraFocus, Qt::NoModifier,
                                                       0, 0, 0, QString(), false, 1, false);

    if (d->cameraPressed)
        QWindowSystemInterface::handleExtendedKeyEvent(window, QEvent::KeyRelease, Qt::Key_Camera, Qt::NoModifier,
                                                       0, 0, 0, QString(), false, 1, false);
    d->cameraHalfPressed = false;
    d->cameraPressed = false;
    return S_OK;
}
#endif // WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)

HRESULT QWinRTIntegration::onSuspended(IInspectable *, ISuspendingEventArgs *)
{
    QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationSuspended);
    QWindowSystemInterface::flushWindowSystemEvents();
    return S_OK;
}

HRESULT QWinRTIntegration::onResume(IInspectable *, IInspectable *)
{
    // First the system invokes onResume and then changes
    // the visibility of the screen to be active.
    QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationHidden);
    return S_OK;
}

QPlatformOffscreenSurface *QWinRTIntegration::createPlatformOffscreenSurface(QOffscreenSurface *surface) const
{
    QEGLPbuffer *pbuffer = nullptr;
    HRESULT hr = QEventDispatcherWinRT::runOnXamlThread([&pbuffer, surface]() {
        pbuffer = new QEGLPbuffer(QWinRTEGLContext::display(), surface->requestedFormat(), surface);
        return S_OK;
    });
    if (hr == UI_E_WINDOW_CLOSED) {
        // This is only used for shutdown of applications.
        // In case we do not return an empty surface the scenegraph will try
        // to create a new native window during application exit causing crashes
        // or assertions.
        return new QPlatformOffscreenSurface(surface);
    }

    return pbuffer;
}


QT_END_NAMESPACE
