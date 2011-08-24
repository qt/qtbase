/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwindowsintegration.h"
#include "qwindowsbackingstore.h"
#include "qwindowswindow.h"
#include "qwindowscontext.h"
#include "qwindowsglcontext.h"
#include "qwindowsscreen.h"
#include "qwindowsfontdatabase.h"
#include "qwindowsguieventdispatcher.h"
#include "qwindowsclipboard.h"
#include "qwindowsdrag.h"
#include "qwindowsinputcontext.h"

#include <QtGui/QPlatformNativeInterface>
#include <QtGui/QWindowSystemInterface>
#include <QtGui/QBackingStore>
#include <QtGui/private/qpixmap_raster_p.h>
#include <QtGui/private/qguiapplication_p.h>

#include <QtCore/private/qeventdispatcher_win_p.h>
#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

/*!
    \class QWindowsNativeInterface
    \brief Provides access to native handles.

    Currently implemented keys
    \list
    \o handle (HWND)
    \o getDC (DC)
    \o releaseDC Releases the previously acquired DC and returns 0.
    \endlist

    \ingroup qt-lighthouse-win
*/

class QWindowsNativeInterface : public QPlatformNativeInterface
{
public:
    virtual void *nativeResourceForWindow(const QByteArray &resource, QWindow *window);
    virtual void *nativeResourceForBackingStore(const QByteArray &resource, QBackingStore *bs);
};

void *QWindowsNativeInterface::nativeResourceForWindow(const QByteArray &resource, QWindow *window)
{
    if (!window || !window->handle()) {
        qWarning("%s: '%s' requested for null window or window without handle.", __FUNCTION__, resource.constData());
        return 0;
    }
    QWindowsWindow *bw = static_cast<QWindowsWindow *>(window->handle());
    if (resource == "handle")
        return bw->handle();
    if (window->surfaceType() == QWindow::RasterSurface) {
        if (resource == "getDC")
            return bw->getDC();
        if (resource == "releaseDC") {
            bw->releaseDC();
            return 0;
        }
    }
    qWarning("%s: Invalid key '%s' requested.", __FUNCTION__, resource.constData());
    return 0;
}

void *QWindowsNativeInterface::nativeResourceForBackingStore(const QByteArray &resource, QBackingStore *bs)
{
    if (!bs || !bs->handle()) {
        qWarning("%s: '%s' requested for null backingstore or backingstore without handle.", __FUNCTION__, resource.constData());
        return 0;
    }
    QWindowsBackingStore *wbs = static_cast<QWindowsBackingStore *>(bs->handle());
    if (resource == "getDC")
        return wbs->getDC();
    qWarning("%s: Invalid key '%s' requested.", __FUNCTION__, resource.constData());
    return 0;
}

/*!
    \class QWindowsIntegration
    \brief QPlatformIntegration implementation for Windows.
    \ingroup qt-lighthouse-win
*/

struct QWindowsIntegrationPrivate
{
    typedef QSharedPointer<QOpenGLStaticContext> QOpenGLStaticContextPtr;

    explicit QWindowsIntegrationPrivate(bool openGL);

    const bool m_openGL;
    QWindowsContext m_context;
    QWindowsFontDatabase m_fontDatabase;
    QWindowsNativeInterface m_nativeInterface;
    QWindowsClipboard m_clipboard;
    QWindowsDrag m_drag;
    QWindowsGuiEventDispatcher *m_eventDispatcher;
    QOpenGLStaticContextPtr m_staticOpenGLContext;
    QWindowsInputContext m_inputContext;
};

QWindowsIntegrationPrivate::QWindowsIntegrationPrivate(bool openGL)
    : m_openGL(openGL)
    , m_context(openGL)
    , m_eventDispatcher(new QWindowsGuiEventDispatcher)
{
}

QWindowsIntegration::QWindowsIntegration(bool openGL) :
    d(new QWindowsIntegrationPrivate(openGL))
{
    QGuiApplicationPrivate::instance()->setEventDispatcher(d->m_eventDispatcher);
    d->m_clipboard.registerViewer();
    foreach (QPlatformScreen *pscr, QWindowsScreen::screens())
        screenAdded(pscr);
}

QWindowsIntegration::~QWindowsIntegration()
{
    if (QWindowsContext::verboseIntegration)
        qDebug("%s", __FUNCTION__);
}

bool QWindowsIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps:
        return true;
    default:
        return QPlatformIntegration::hasCapability(cap);
    }
    return false;
}

QPlatformPixmap *QWindowsIntegration::createPlatformPixmap(QPlatformPixmap::PixelType type) const
{
    if (QWindowsContext::verboseIntegration)
        qDebug() << __FUNCTION__ << type;
    return new QRasterPlatformPixmap(type);
}

QPlatformWindow *QWindowsIntegration::createPlatformWindow(QWindow *window) const
{
    const bool isGL = window->surfaceType() == QWindow::OpenGLSurface;
    QWindowsWindow::WindowData requested;
    requested.flags = window->windowFlags();
    requested.geometry = window->geometry();
    const QWindowsWindow::WindowData obtained
            = QWindowsWindow::WindowData::create(window, requested, window->windowTitle(), isGL);
    if (QWindowsContext::verboseIntegration || QWindowsContext::verboseWindows)
        qDebug().nospace()
            << __FUNCTION__ << ' ' << window << '\n'
            << "    Requested: " << requested.geometry << " Flags="
            << QWindowsWindow::debugWindowFlags(requested.flags) << '\n'
            << "    Obtained : " << obtained.geometry << " Margins "
            << obtained.frame  << " Flags="
            << QWindowsWindow::debugWindowFlags(obtained.flags)
            << " Handle=" << obtained.hwnd << '\n';
    if (!obtained.hwnd)
        return 0;
    if (requested.flags != obtained.flags)
        window->setWindowFlags(obtained.flags);
    if (requested.geometry != obtained.geometry)
        QWindowSystemInterface::handleGeometryChange(window, obtained.geometry);
    return new QWindowsWindow(window, obtained);
}

QPlatformBackingStore *QWindowsIntegration::createPlatformBackingStore(QWindow *window) const
{
    if (QWindowsContext::verboseIntegration)
        qDebug() << __FUNCTION__ << window;
    return new QWindowsBackingStore(window);
}

QPlatformGLContext
    *QWindowsIntegration::createPlatformGLContext(QGuiGLContext *context) const
{
    if (QWindowsContext::verboseIntegration)
        qDebug() << __FUNCTION__ << context->format();
    if (d->m_staticOpenGLContext.isNull())
        d->m_staticOpenGLContext =
            QSharedPointer<QOpenGLStaticContext>(QOpenGLStaticContext::create());
    QScopedPointer<QWindowsGLContext> result(new QWindowsGLContext(d->m_staticOpenGLContext, context));
    if (result->isValid())
        return result.take();
    return 0;
}

QPlatformFontDatabase *QWindowsIntegration::fontDatabase() const
{
    return &d->m_fontDatabase;
}

QPlatformNativeInterface *QWindowsIntegration::nativeInterface() const
{
    return &d->m_nativeInterface;
}

QPlatformClipboard * QWindowsIntegration::clipboard() const
{
    return &d->m_clipboard;
}

QPlatformDrag *QWindowsIntegration::drag() const
{
    if (QWindowsContext::verboseIntegration)
        qDebug("%s", __FUNCTION__ );
    return &d->m_drag;
}

QPlatformInputContext * QWindowsIntegration::inputContext() const
{
    return &d->m_inputContext;
}

QWindowsIntegration *QWindowsIntegration::instance()
{
    return static_cast<QWindowsIntegration *>(QGuiApplicationPrivate::platformIntegration());
}

QAbstractEventDispatcher * QWindowsIntegration::guiThreadEventDispatcher() const
{
    return d->m_eventDispatcher;
}

QT_END_NAMESPACE
