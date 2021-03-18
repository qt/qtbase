/***************************************************************************
**
** Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
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
#include <QtEventDispatcherSupport/private/qgenericunixeventdispatcher_p.h>

#include <Application.h>

QT_BEGIN_NAMESPACE

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

    const QString signature = QStringLiteral("application/x-vnd.Qt.%1").arg(QFileInfo(QCoreApplication::applicationFilePath()).fileName());

    QHaikuApplication *app = new QHaikuApplication(signature.toLocal8Bit());
    be_app = app;

    const thread_id applicationThreadId = spawn_thread(startApplicationThread, "app_thread", 1, static_cast<void*>(app));
    resume_thread(applicationThreadId);
    app->UnlockLooper();

    m_screen = new QHaikuScreen;

    m_services = new QHaikuServices;

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
