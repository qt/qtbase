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

#include <QtCore/QCoreApplication>
#include <QtGui/QSurface>
#include <QtGui/QOpenGLContext>
#include <qfunctions_winrt.h>

#include <functional>
#include <wrl.h>
#include <windows.ui.xaml.h>
#include <windows.applicationmodel.h>
#include <windows.applicationmodel.core.h>
#include <windows.ui.core.h>
#include <windows.ui.viewmanagement.h>
#include <windows.graphics.display.h>
#ifdef Q_OS_WINPHONE
#  include <windows.phone.ui.input.h>
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
#ifdef Q_OS_WINPHONE
using namespace ABI::Windows::Phone::UI::Input;
#endif

typedef IEventHandler<IInspectable *> ResumeHandler;
typedef IEventHandler<SuspendingEventArgs *> SuspendHandler;
#ifdef Q_OS_WINPHONE
typedef IEventHandler<BackPressedEventArgs*> BackPressedHandler;
#endif

QT_BEGIN_NAMESPACE

typedef HRESULT (__stdcall ICoreApplication::*CoreApplicationCallbackRemover)(EventRegistrationToken);
uint qHash(CoreApplicationCallbackRemover key) { void *ptr = *(void **)(&key); return qHash(ptr); }
#ifdef Q_OS_WINPHONE
typedef HRESULT (__stdcall IHardwareButtonsStatics::*HardwareButtonsCallbackRemover)(EventRegistrationToken);
uint qHash(HardwareButtonsCallbackRemover key) { void *ptr = *(void **)(&key); return qHash(ptr); }
#endif

class QWinRTIntegrationPrivate
{
public:
    QPlatformFontDatabase *fontDatabase;
    QPlatformServices *platformServices;
    QWinRTScreen *mainScreen;
    QScopedPointer<QWinRTInputContext> inputContext;

    ComPtr<ICoreApplication> application;
    QHash<CoreApplicationCallbackRemover, EventRegistrationToken> applicationTokens;
#ifdef Q_OS_WINPHONE
    ComPtr<IHardwareButtonsStatics> hardwareButtons;
    QHash<HardwareButtonsCallbackRemover, EventRegistrationToken> buttonsTokens;
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
                                        &d->applicationTokens[&ICoreApplication::remove_Resuming]);
    Q_ASSERT_SUCCEEDED(hr);
    hr = d->application->add_Resuming(Callback<ResumeHandler>(this, &QWinRTIntegration::onResume).Get(),
                                      &d->applicationTokens[&ICoreApplication::remove_Resuming]);
    Q_ASSERT_SUCCEEDED(hr);

#ifdef Q_OS_WINPHONE
    hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Phone_UI_Input_HardwareButtons).Get(),
                                IID_PPV_ARGS(&d->hardwareButtons));
    Q_ASSERT_SUCCEEDED(hr);
    hr = d->hardwareButtons->add_BackPressed(Callback<BackPressedHandler>(this, &QWinRTIntegration::onBackButtonPressed).Get(),
                                             &d->buttonsTokens[&IHardwareButtonsStatics::remove_BackPressed]);
    Q_ASSERT_SUCCEEDED(hr);
#endif // Q_OS_WINPHONE

    hr = QEventDispatcherWinRT::runOnXamlThread([this, d]() {
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

        d->mainScreen = new QWinRTScreen(window.Get());
        d->inputContext.reset(new QWinRTInputContext(d->mainScreen));
        screenAdded(d->mainScreen);
        return S_OK;
    });
    Q_ASSERT_SUCCEEDED(hr);
}

QWinRTIntegration::~QWinRTIntegration()
{
    Q_D(QWinRTIntegration);
    HRESULT hr;
#ifdef Q_OS_WINPHONE
    for (QHash<HardwareButtonsCallbackRemover, EventRegistrationToken>::const_iterator i = d->buttonsTokens.begin(); i != d->buttonsTokens.end(); ++i) {
        hr = (d->hardwareButtons.Get()->*i.key())(i.value());
        Q_ASSERT_SUCCEEDED(hr);
    }
#endif
    for (QHash<CoreApplicationCallbackRemover, EventRegistrationToken>::const_iterator i = d->applicationTokens.begin(); i != d->applicationTokens.end(); ++i) {
        hr = (d->application.Get()->*i.key())(i.value());
        Q_ASSERT_SUCCEEDED(hr);
    }
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

bool QWinRTIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps:
    case OpenGL:
    case ApplicationState:
        return true;
    case NonFullScreenWindows:
        return false;
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

Qt::KeyboardModifiers QWinRTIntegration::queryKeyboardModifiers() const
{
    Q_D(const QWinRTIntegration);
    return d->mainScreen->keyboardModifiers();
}

QStringList QWinRTIntegration::themeNames() const
{
    return QStringList(QLatin1String("winrt"));
}

QPlatformTheme *QWinRTIntegration::createPlatformTheme(const QString &
name) const
{
    if (name == QLatin1String("winrt"))
        return new QWinRTTheme();

    return 0;
}

// System-level integration points

#ifdef Q_OS_WINPHONE
HRESULT QWinRTIntegration::onBackButtonPressed(IInspectable *, IBackPressedEventArgs *args)
{
    Q_D(QWinRTIntegration);

    QKeyEvent backPress(QEvent::KeyPress, Qt::Key_Back, Qt::NoModifier);
    QKeyEvent backRelease(QEvent::KeyRelease, Qt::Key_Back, Qt::NoModifier);
    backPress.setAccepted(false);
    backRelease.setAccepted(false);

    QWindow *window = d->mainScreen->topWindow();
    QObject *receiver = window ? static_cast<QObject *>(window)
                               : static_cast<QObject *>(QCoreApplication::instance());

    // If the event is ignored, the app go to the background
    QCoreApplication::sendEvent(receiver, &backPress);
    QCoreApplication::sendEvent(receiver, &backRelease);
    args->put_Handled(backPress.isAccepted() || backRelease.isAccepted());

    return S_OK;
}
#endif // Q_OS_WINPHONE

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

QT_END_NAMESPACE
