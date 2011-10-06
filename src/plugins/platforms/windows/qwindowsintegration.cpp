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

#include "qwindowsintegration.h"
#include "qwindowsbackingstore.h"
#include "qwindowswindow.h"
#include "qwindowscontext.h"
#include "qwindowsglcontext.h"
#include "qwindowsscreen.h"
#include "qwindowstheme.h"
#include "qwindowsservices.h"
#ifndef QT_NO_FREETYPE
#include "qwindowsfontdatabase_ft.h"
#endif
#include "qwindowsfontdatabase.h"
#include "qwindowsguieventdispatcher.h"
#include "qwindowsclipboard.h"
#include "qwindowsdrag.h"
#include "qwindowsinputcontext.h"
#include "qwindowskeymapper.h"
#include "accessible/qwindowsaccessibility.h"

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
    \li handle (HWND)
    \li getDC (DC)
    \li releaseDC Releases the previously acquired DC and returns 0.
    \endlist

    \ingroup qt-lighthouse-win
*/

class QWindowsNativeInterface : public QPlatformNativeInterface
{
    Q_OBJECT
public:
    virtual void *nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context);
    virtual void *nativeResourceForWindow(const QByteArray &resource, QWindow *window);
    virtual void *nativeResourceForBackingStore(const QByteArray &resource, QBackingStore *bs);
    virtual EventFilter setEventFilter(const QByteArray &eventType, EventFilter filter)
        { return QWindowsContext::instance()->setEventFilter(eventType, filter); }

    Q_INVOKABLE void *createMessageWindow(const QString &classNameTemplate,
                                          const QString &windowName,
                                          void *eventProc) const;
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

void *QWindowsNativeInterface::nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context)
{
    if (!context || !context->handle()) {
        qWarning("%s: '%s' requested for null context or context without handle.", __FUNCTION__, resource.constData());
        return 0;
    }
    QWindowsGLContext *windowsContext = static_cast<QWindowsGLContext *>(context->handle());
    if (resource == "renderingContext")
        return windowsContext->renderingContext();

    qWarning("%s: Invalid key '%s' requested.", __FUNCTION__, resource.constData());
    return 0;
}

/*!
    \brief Creates a non-visible window handle for filtering messages.
*/

void *QWindowsNativeInterface::createMessageWindow(const QString &classNameTemplate,
                                                   const QString &windowName,
                                                   void *eventProc) const
{
    QWindowsContext *ctx = QWindowsContext::instance();
    const HWND hwnd = ctx->createDummyWindow(classNameTemplate,
                                             (wchar_t*)windowName.utf16(),
                                             (WNDPROC)eventProc);
    return hwnd;
}

/*!
    \class QWindowsIntegration
    \brief QPlatformIntegration implementation for Windows.
    \ingroup qt-lighthouse-win
*/

struct QWindowsIntegrationPrivate
{
    typedef QSharedPointer<QOpenGLStaticContext> QOpenGLStaticContextPtr;

    QWindowsIntegrationPrivate();
    ~QWindowsIntegrationPrivate();

    QWindowsContext m_context;
    QPlatformFontDatabase *m_fontDatabase;
    QWindowsNativeInterface m_nativeInterface;
    QWindowsClipboard m_clipboard;
    QWindowsDrag m_drag;
    QWindowsGuiEventDispatcher *m_eventDispatcher;
    QOpenGLStaticContextPtr m_staticOpenGLContext;
    QWindowsInputContext m_inputContext;
    QWindowsAccessibility m_accessibility;
    QWindowsTheme m_theme;
    QWindowsServices m_services;
};

QWindowsIntegrationPrivate::QWindowsIntegrationPrivate()
    : m_fontDatabase(0), m_eventDispatcher(new QWindowsGuiEventDispatcher)
{
}

QWindowsIntegrationPrivate::~QWindowsIntegrationPrivate()
{
    if (m_fontDatabase)
        delete m_fontDatabase;
}

QWindowsIntegration::QWindowsIntegration() :
    d(new QWindowsIntegrationPrivate)
{
    QGuiApplicationPrivate::instance()->setEventDispatcher(d->m_eventDispatcher);
    d->m_clipboard.registerViewer();
    d->m_context.screenManager().handleScreenChanges();
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
    case OpenGL:
        return true;
    case ThreadedOpenGL:
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
    QWindowsWindow::WindowData requested;
    requested.flags = window->windowFlags();
    requested.geometry = window->geometry();
    const QWindowsWindow::WindowData obtained
            = QWindowsWindow::WindowData::create(window, requested, window->windowTitle());
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

QPlatformOpenGLContext
    *QWindowsIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
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
    if (!d->m_fontDatabase) {
#ifndef QT_NO_FREETYPE
        if (d->m_nativeInterface.property("fontengine").toString() == QLatin1String("native"))
            d->m_fontDatabase = new QWindowsFontDatabase();
        else
            d->m_fontDatabase = new QWindowsFontDatabaseFT();
#else
        d->m_fontDatabase = new QWindowsFontDatabase();
#endif
    }
    return d->m_fontDatabase;
}

static inline int keyBoardAutoRepeatRateMS()
{
  DWORD time = 0;
  if (SystemParametersInfo(SPI_GETKEYBOARDSPEED, 0, &time, 0))
      return time ? 1000 / static_cast<int>(time) : 500;
  return 30;
}

QVariant QWindowsIntegration::styleHint(QPlatformIntegration::StyleHint hint) const
{
    switch (hint) {
    case QPlatformIntegration::CursorFlashTime:
        if (const unsigned timeMS = GetCaretBlinkTime())
            return QVariant(int(timeMS));
        break;
    case KeyboardAutoRepeatRate:
        return QVariant(keyBoardAutoRepeatRateMS());
    case QPlatformIntegration::StartDragTime:
    case QPlatformIntegration::StartDragDistance:
    case QPlatformIntegration::MouseDoubleClickInterval:
    case QPlatformIntegration::KeyboardInputInterval:
    case QPlatformIntegration::ShowIsFullScreen:
        break; // Not implemented
    }
    return QPlatformIntegration::styleHint(hint);
}

Qt::KeyboardModifiers QWindowsIntegration::queryKeyboardModifiers() const
{
    return QWindowsKeyMapper::queryKeyboardModifiers();
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
    return &d->m_drag;
}

QPlatformInputContext * QWindowsIntegration::inputContext() const
{
    return &d->m_inputContext;
}

QPlatformAccessibility *QWindowsIntegration::accessibility() const
{
    return &d->m_accessibility;
}

QWindowsIntegration *QWindowsIntegration::instance()
{
    return static_cast<QWindowsIntegration *>(QGuiApplicationPrivate::platformIntegration());
}

QAbstractEventDispatcher * QWindowsIntegration::guiThreadEventDispatcher() const
{
    return d->m_eventDispatcher;
}

QPlatformTheme *QWindowsIntegration::platformTheme() const
{
    return &d->m_theme;
}

QPlatformServices *QWindowsIntegration::services() const
{
    return &d->m_services;
}

QT_END_NAMESPACE

#include "qwindowsintegration.moc"
