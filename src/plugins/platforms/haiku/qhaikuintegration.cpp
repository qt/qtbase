/***************************************************************************
**
** Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
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
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
#include <QtPlatformSupport/private/qgenericunixeventdispatcher_p.h>

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
    screenAdded(m_screen);
}

QHaikuIntegration::~QHaikuIntegration()
{
    destroyScreen(m_screen);
    m_screen = Q_NULLPTR;

    delete m_services;
    m_services = Q_NULLPTR;

    delete m_clipboard;
    m_clipboard = Q_NULLPTR;

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
