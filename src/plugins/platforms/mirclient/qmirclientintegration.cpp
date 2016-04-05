/****************************************************************************
**
** Copyright (C) 2014-2015 Canonical, Ltd.
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


// Local
#include "qmirclientintegration.h"
#include "qmirclientbackingstore.h"
#include "qmirclientclipboard.h"
#include "qmirclientglcontext.h"
#include "qmirclientinput.h"
#include "qmirclientlogging.h"
#include "qmirclientnativeinterface.h"
#include "qmirclientscreen.h"
#include "qmirclienttheme.h"
#include "qmirclientwindow.h"

// Qt
#include <QGuiApplication>
#include <private/qguiapplication_p.h>
#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatforminputcontextfactory_p.h>
#include <qpa/qplatforminputcontext.h>
#include <QtPlatformSupport/private/qgenericunixfontdatabase_p.h>
#include <QtPlatformSupport/private/qgenericunixeventdispatcher_p.h>
#include <QOpenGLContext>

// platform-api
#include <ubuntu/application/lifecycle_delegate.h>
#include <ubuntu/application/id.h>
#include <ubuntu/application/options.h>

static void resumedCallback(const UApplicationOptions *options, void* context)
{
    Q_UNUSED(options)
    Q_UNUSED(context)
    DASSERT(context != NULL);
    if (qGuiApp->focusWindow()) {
        QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationActive);
    } else {
        QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationInactive);
    }
}

static void aboutToStopCallback(UApplicationArchive *archive, void* context)
{
    Q_UNUSED(archive)
    DASSERT(context != NULL);
    QMirClientClientIntegration* integration = static_cast<QMirClientClientIntegration*>(context);
    QPlatformInputContext *inputContext = integration->inputContext();
    if (inputContext) {
        inputContext->hideInputPanel();
    } else {
        qWarning("QMirClientClientIntegration aboutToStopCallback(): no input context");
    }
    QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationSuspended);
}

QMirClientClientIntegration::QMirClientClientIntegration()
    : QPlatformIntegration()
    , mNativeInterface(new QMirClientNativeInterface)
    , mFontDb(new QGenericUnixFontDatabase)
    , mServices(new QMirClientPlatformServices)
    , mClipboard(new QMirClientClipboard)
    , mScaleFactor(1.0)
{
    setupOptions();
    setupDescription();

    // Create new application instance
    mInstance = u_application_instance_new_from_description_with_options(mDesc, mOptions);

    if (Q_UNLIKELY(!mInstance))
        qFatal("QMirClientClientIntegration: connection to Mir server failed. Check that a Mir server is\n"
               "running, and the correct socket is being used and is accessible. The shell may have\n"
               "rejected the incoming connection, so check its log file");

    mNativeInterface->setMirConnection(u_application_instance_get_mir_connection(mInstance));

    // Create default screen.
    screenAdded(new QMirClientScreen(u_application_instance_get_mir_connection(mInstance)));

    // Initialize input.
    if (qEnvironmentVariableIsEmpty("QTUBUNTU_NO_INPUT")) {
        mInput = new QMirClientInput(this);
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

QMirClientClientIntegration::~QMirClientClientIntegration()
{
    delete mInput;
    delete mInputContext;
    for (QScreen *screen : QGuiApplication::screens())
        QPlatformIntegration::destroyScreen(screen->handle());
    delete mServices;
}

QPlatformServices *QMirClientClientIntegration::services() const
{
    return mServices;
}

void QMirClientClientIntegration::setupOptions()
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

void QMirClientClientIntegration::setupDescription()
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

QPlatformWindow* QMirClientClientIntegration::createPlatformWindow(QWindow* window) const
{
    return const_cast<QMirClientClientIntegration*>(this)->createPlatformWindow(window);
}

QPlatformWindow* QMirClientClientIntegration::createPlatformWindow(QWindow* window)
{
    return new QMirClientWindow(window, mClipboard, screen(),
                                mInput, u_application_instance_get_mir_connection(mInstance));
}

QMirClientScreen *QMirClientClientIntegration::screen() const
{
    return static_cast<QMirClientScreen *>(QGuiApplication::primaryScreen()->handle());
}

bool QMirClientClientIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps:
        return true;

    case OpenGL:
        return true;

    case ApplicationState:
        return true;

    case ThreadedOpenGL:
        if (qEnvironmentVariableIsEmpty("QTUBUNTU_NO_THREADED_OPENGL")) {
            return true;
        } else {
            DLOG("ubuntumirclient: disabled threaded OpenGL");
            return false;
        }
    case MultipleWindows:
    case NonFullScreenWindows:
        return true;
    default:
        return QPlatformIntegration::hasCapability(cap);
    }
}

QAbstractEventDispatcher *QMirClientClientIntegration::createEventDispatcher() const
{
    return createUnixEventDispatcher();
}

QPlatformBackingStore* QMirClientClientIntegration::createPlatformBackingStore(QWindow* window) const
{
    return new QMirClientBackingStore(window);
}

QPlatformOpenGLContext* QMirClientClientIntegration::createPlatformOpenGLContext(
        QOpenGLContext* context) const
{
    return const_cast<QMirClientClientIntegration*>(this)->createPlatformOpenGLContext(context);
}

QPlatformOpenGLContext* QMirClientClientIntegration::createPlatformOpenGLContext(
        QOpenGLContext* context)
{
    return new QMirClientOpenGLContext(static_cast<QMirClientScreen*>(context->screen()->handle()),
                                   static_cast<QMirClientOpenGLContext*>(context->shareHandle()));
}

QStringList QMirClientClientIntegration::themeNames() const
{
    return QStringList(QMirClientTheme::name);
}

QPlatformTheme* QMirClientClientIntegration::createPlatformTheme(const QString& name) const
{
    Q_UNUSED(name);
    return new QMirClientTheme;
}

QVariant QMirClientClientIntegration::styleHint(StyleHint hint) const
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

QPlatformClipboard* QMirClientClientIntegration::clipboard() const
{
    return mClipboard.data();
}

QPlatformNativeInterface* QMirClientClientIntegration::nativeInterface() const
{
    return mNativeInterface;
}
