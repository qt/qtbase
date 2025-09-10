// Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qhaikuintegration.h"

#include "qhaikuapplication.h"
#include "qhaikuclipboard.h"
#include "qhaikurasterbackingstore.h"
#include "qhaikurasterwindow.h"
#include "qhaikuscreen.h"
#include "qhaikuservices.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <qpa/qplatformwindow.h>
#include <qpa/qwindowsysteminterface.h>
#include <QtGui/private/qgenericunixeventdispatcher_p.h>

#include <Application.h>

QT_BEGIN_NAMESPACE

using namespace Qt::Literals::StringLiterals;

static long int startApplicationThread(void *data)
{
    QHaikuApplication *app = static_cast<QHaikuApplication*>(data);
    app->LockLooper();
    return app->Run();
}

QHaikuIntegration::QHaikuIntegration(const QStringList &parameters)
    : m_clipboard(new QHaikuClipboard)
{
    Q_UNUSED(parameters);

    const QString signature = "application/x-vnd.Qt.%1"_L1.arg(QFileInfo(QCoreApplication::applicationFilePath()).fileName());

    QHaikuApplication *app = new QHaikuApplication(signature.toLocal8Bit());
    be_app = app;

    const thread_id applicationThreadId = spawn_thread(startApplicationThread, "app_thread", 1, static_cast<void*>(app));
    resume_thread(applicationThreadId);
    app->UnlockLooper();

    m_screen = new QHaikuScreen;

    // notify system about available screen
    QWindowSystemInterface::handleScreenAdded(m_screen);
}

QHaikuIntegration::~QHaikuIntegration()
{
    QWindowSystemInterface::handleScreenRemoved(m_screen);
    m_screen = nullptr;

    delete m_services;
    m_services = nullptr;

    delete m_clipboard;
    m_clipboard = nullptr;

    be_app->LockLooper();
    be_app->Quit();
}

bool QHaikuIntegration::hasCapability(QPlatformIntegration::Capability capability) const
{
    return QPlatformIntegration::hasCapability(capability);
}

QPlatformFontDatabase *QHaikuIntegration::fontDatabase() const
{
    return QPlatformIntegration::fontDatabase();
}

QPlatformServices *QHaikuIntegration::services() const
{
    if (!m_services)
        m_services = new QHaikuServices;

    return m_services;
}

QPlatformClipboard *QHaikuIntegration::clipboard() const
{
    return m_clipboard;
}

QPlatformWindow *QHaikuIntegration::createPlatformWindow(QWindow *window) const
{
    QPlatformWindow *platformWindow = new QHaikuRasterWindow(window);
    platformWindow->requestActivateWindow();
    return platformWindow;
}

QPlatformBackingStore *QHaikuIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new QHaikuRasterBackingStore(window);
}

QAbstractEventDispatcher *QHaikuIntegration::createEventDispatcher() const
{
    return createUnixEventDispatcher();
}

QT_END_NAMESPACE
