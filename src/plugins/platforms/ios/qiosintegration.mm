/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qiosintegration.h"
#include "qioswindow.h"
#include "qiosbackingstore.h"
#include "qiosscreen.h"
#include "qioseventdispatcher.h"
#include "qioscontext.h"

#include <QtPlatformSupport/private/qcoretextfontdatabase_p.h>

#include <QtDebug>

QT_BEGIN_NAMESPACE

QIOSIntegration::QIOSIntegration()
    : m_fontDatabase(new QCoreTextFontDatabase)
    , m_screen(new QIOSScreen(QIOSScreen::MainScreen))
{
    screenAdded(m_screen);
}

QIOSIntegration::~QIOSIntegration()
{
}

QPlatformPixmap *QIOSIntegration::createPlatformPixmap(QPlatformPixmap::PixelType type) const
{
    Q_UNUSED(type);

    qDebug() << __FUNCTION__ << "not yet implemented";
    return 0;
}

QPlatformWindow *QIOSIntegration::createPlatformWindow(QWindow *window) const
{
    qDebug() <<  __FUNCTION__ << "Creating platform window";
    return new QIOSWindow(window);
}

QPlatformBackingStore *QIOSIntegration::createPlatformBackingStore(QWindow *window) const
{
    qDebug() <<  __FUNCTION__ << "Creating platform backingstore";
    return new QIOSBackingStore(window);
}

// Used when the QWindow's surface type is set by the client to QSurface::OpenGLSurface
QPlatformOpenGLContext *QIOSIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    Q_UNUSED(context);
    qDebug() <<  __FUNCTION__ << "Creating platform opengl context";
    return new QIOSContext(context);
}

QAbstractEventDispatcher *QIOSIntegration::guiThreadEventDispatcher() const
{
    return new QIOSEventDispatcher();
}

QPlatformFontDatabase * QIOSIntegration::fontDatabase() const
{
    return m_fontDatabase;
}

QPlatformNativeInterface *QIOSIntegration::nativeInterface() const
{
    return const_cast<QIOSIntegration *>(this);
}

void *QIOSIntegration::nativeResourceForWindow(const QByteArray &resource, QWindow *window)
{
    if (!window || !window->handle())
        return 0;

    QByteArray lowerCaseResource = resource.toLower();

    QIOSWindow *platformWindow = static_cast<QIOSWindow *>(window->handle());

    if (lowerCaseResource == "uiview")
        return platformWindow->nativeView();

    return 0;
}

QT_END_NAMESPACE
