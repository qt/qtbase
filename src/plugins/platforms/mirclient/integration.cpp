/*
 * Copyright (C) 2014-2015 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
 * SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Qt
#include <QGuiApplication>
#include <private/qguiapplication_p.h>
#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatforminputcontextfactory_p.h>
#include <qpa/qplatforminputcontext.h>
#include <QtPlatformSupport/private/qgenericunixfontdatabase_p.h>
#include <QtPlatformSupport/private/qgenericunixeventdispatcher_p.h>
#include <QOpenGLContext>

// Local
#include "backingstore.h"
#include "clipboard.h"
#include "glcontext.h"
#include "input.h"
#include "integration.h"
#include "logging.h"
#include "nativeinterface.h"
#include "screen.h"
#include "theme.h"
#include "window.h"

// platform-api
#include <ubuntu/application/lifecycle_delegate.h>
#include <ubuntu/application/id.h>
#include <ubuntu/application/options.h>

static void resumedCallback(const UApplicationOptions *options, void* context)
{
    Q_UNUSED(options)
    Q_UNUSED(context)
    DASSERT(context != NULL);
    QCoreApplication::postEvent(QCoreApplication::instance(),
                                new QEvent(QEvent::ApplicationActivate));
}

static void aboutToStopCallback(UApplicationArchive *archive, void* context)
{
    Q_UNUSED(archive)
    DASSERT(context != NULL);
    UbuntuClientIntegration* integration = static_cast<UbuntuClientIntegration*>(context);
    integration->inputContext()->hideInputPanel();
    QCoreApplication::postEvent(QCoreApplication::instance(),
                                new QEvent(QEvent::ApplicationDeactivate));
}

UbuntuClientIntegration::UbuntuClientIntegration()
    : QPlatformIntegration()
    , mNativeInterface(new UbuntuNativeInterface)
    , mFontDb(new QGenericUnixFontDatabase)
    , mServices(new UbuntuPlatformServices)
    , mClipboard(new UbuntuClipboard)
    , mScaleFactor(1.0)
{
    setupOptions();
    setupDescription();

    // Create new application instance
    mInstance = u_application_instance_new_from_description_with_options(mDesc, mOptions);

    if (mInstance == nullptr)
        qFatal("UbuntuClientIntegration: connection to Mir server failed. Check that a Mir server is\n"
               "running, and the correct socket is being used and is accessible. The shell may have\n"
               "rejected the incoming connection, so check its log file");

    // Create default screen.
    mScreen = new UbuntuScreen(u_application_instance_get_mir_connection(mInstance));
    screenAdded(mScreen);

    // Initialize input.
    if (qEnvironmentVariableIsEmpty("QTUBUNTU_NO_INPUT")) {
        mInput = new UbuntuInput(this);
        mInputContext = QPlatformInputContextFactory::create();
    } else {
        mInput = nullptr;
        mInputContext = nullptr;
    }

    // compute the scale factor
    const int defaultGridUnit = 8;
    int gridUnit = defaultGridUnit;
    QByteArray gridUnitString = qgetenv("GRID_UNIT_PX");
    if (!gridUnitString.isEmpty()) {
        bool ok;
        gridUnit = gridUnitString.toInt(&ok);
        if (!ok) {
            gridUnit = defaultGridUnit;
        }
    }
    mScaleFactor = static_cast<qreal>(gridUnit) / defaultGridUnit;
}

UbuntuClientIntegration::~UbuntuClientIntegration()
{
    delete mInput;
    delete mInputContext;
    delete mScreen;
    delete mServices;
}

QPlatformServices *UbuntuClientIntegration::services() const
{
    return mServices;
}

void UbuntuClientIntegration::setupOptions()
{
    QStringList args = QCoreApplication::arguments();
    int argc = args.size() + 1;
    char **argv = new char*[argc];
    for (int i = 0; i < argc - 1; i++)
        argv[i] = qstrdup(args.at(i).toLocal8Bit());
    argv[argc - 1] = nullptr;

    mOptions = u_application_options_new_from_cmd_line(argc - 1, argv);

    for (int i = 0; i < argc; i++)
        delete [] argv[i];
    delete [] argv;
}

void UbuntuClientIntegration::setupDescription()
{
    mDesc = u_application_description_new();
    UApplicationId* id = u_application_id_new_from_stringn("QtUbuntu", 8);
    u_application_description_set_application_id(mDesc, id);

    UApplicationLifecycleDelegate* delegate = u_application_lifecycle_delegate_new();
    u_application_lifecycle_delegate_set_application_resumed_cb(delegate, &resumedCallback);
    u_application_lifecycle_delegate_set_application_about_to_stop_cb(delegate, &aboutToStopCallback);
    u_application_lifecycle_delegate_set_context(delegate, this);
    u_application_description_set_application_lifecycle_delegate(mDesc, delegate);
}

QPlatformWindow* UbuntuClientIntegration::createPlatformWindow(QWindow* window) const
{
    return const_cast<UbuntuClientIntegration*>(this)->createPlatformWindow(window);
}

QPlatformWindow* UbuntuClientIntegration::createPlatformWindow(QWindow* window)
{
    QPlatformWindow* platformWindow = new UbuntuWindow(
            window, mClipboard, static_cast<UbuntuScreen*>(mScreen), mInput, u_application_instance_get_mir_connection(mInstance));
    platformWindow->requestActivateWindow();
    return platformWindow;
}

bool UbuntuClientIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps:
        return true;
        break;

    case OpenGL:
        return true;
        break;

    case ThreadedOpenGL:
        if (qEnvironmentVariableIsEmpty("QTUBUNTU_NO_THREADED_OPENGL")) {
            return true;
        } else {
            DLOG("ubuntumirclient: disabled threaded OpenGL");
            return false;
        }
        break;

    default:
        return QPlatformIntegration::hasCapability(cap);
    }
}

QAbstractEventDispatcher *UbuntuClientIntegration::createEventDispatcher() const
{
    return createUnixEventDispatcher();
}

QPlatformBackingStore* UbuntuClientIntegration::createPlatformBackingStore(QWindow* window) const
{
    return new UbuntuBackingStore(window);
}

QPlatformOpenGLContext* UbuntuClientIntegration::createPlatformOpenGLContext(
        QOpenGLContext* context) const
{
    return const_cast<UbuntuClientIntegration*>(this)->createPlatformOpenGLContext(context);
}

QPlatformOpenGLContext* UbuntuClientIntegration::createPlatformOpenGLContext(
        QOpenGLContext* context)
{
    return new UbuntuOpenGLContext(static_cast<UbuntuScreen*>(context->screen()->handle()),
                                   static_cast<UbuntuOpenGLContext*>(context->shareHandle()));
}

QStringList UbuntuClientIntegration::themeNames() const
{
    return QStringList(UbuntuTheme::name);
}

QPlatformTheme* UbuntuClientIntegration::createPlatformTheme(const QString& name) const
{
    Q_UNUSED(name);
    return new UbuntuTheme;
}

QVariant UbuntuClientIntegration::styleHint(StyleHint hint) const
{
    switch (hint) {
        case QPlatformIntegration::StartDragDistance: {
            // default is 10 pixels (see QPlatformTheme::defaultThemeHint)
            return 10.0 * mScaleFactor;
        }
        case QPlatformIntegration::PasswordMaskDelay: {
            // return time in milliseconds - 1 second
            return QVariant(1000);
        }
        default:
            break;
    }
    return QPlatformIntegration::styleHint(hint);
}

QPlatformClipboard* UbuntuClientIntegration::clipboard() const
{
    return mClipboard.data();
}
