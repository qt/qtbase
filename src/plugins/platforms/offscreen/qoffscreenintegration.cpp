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

#include "qoffscreenintegration.h"
#include "qoffscreenwindow.h"
#include "qoffscreencommon.h"

#if defined(Q_OS_UNIX)
#include <QtEventDispatcherSupport/private/qgenericunixeventdispatcher_p.h>
#if defined(Q_OS_MAC)
#include <qpa/qplatformfontdatabase.h>
#include <QtFontDatabaseSupport/private/qcoretextfontdatabase_p.h>
#else
#include <QtFontDatabaseSupport/private/qgenericunixfontdatabase_p.h>
#endif
#elif defined(Q_OS_WIN)
#include <QtFontDatabaseSupport/private/qfreetypefontdatabase_p.h>
#ifndef Q_OS_WINRT
#include <QtCore/private/qeventdispatcher_win_p.h>
#else
#include <QtCore/private/qeventdispatcher_winrt_p.h>
#endif
#endif

#include <QtGui/private/qpixmap_raster_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatforminputcontextfactory_p.h>
#include <qpa/qplatforminputcontext.h>
#include <qpa/qplatformtheme.h>
#include <qpa/qwindowsysteminterface.h>

#include <qpa/qplatformservices.h>

#if QT_CONFIG(xlib) && QT_CONFIG(opengl) && !QT_CONFIG(opengles2)
#include "qoffscreenintegration_x11.h"
#endif

QT_BEGIN_NAMESPACE

class QCoreTextFontEngine;

template <typename BaseEventDispatcher>
class QOffscreenEventDispatcher : public BaseEventDispatcher
{
public:
    explicit QOffscreenEventDispatcher(QObject *parent = nullptr)
        : BaseEventDispatcher(parent)
    {
    }

    bool processEvents(QEventLoop::ProcessEventsFlags flags)
    {
        bool didSendEvents = BaseEventDispatcher::processEvents(flags);

        return QWindowSystemInterface::sendWindowSystemEvents(flags) || didSendEvents;
    }

    bool hasPendingEvents()
    {
        return BaseEventDispatcher::hasPendingEvents()
            || QWindowSystemInterface::windowSystemEventsQueued();
    }

    void flush()
    {
        if (qApp)
            qApp->sendPostedEvents();
        BaseEventDispatcher::flush();
    }
};

QOffscreenIntegration::QOffscreenIntegration()
{
#if defined(Q_OS_UNIX)
#if defined(Q_OS_MAC)
    m_fontDatabase.reset(new QCoreTextFontDatabaseEngineFactory<QCoreTextFontEngine>);
#else
    m_fontDatabase.reset(new QGenericUnixFontDatabase());
#endif
#elif defined(Q_OS_WIN)
    m_fontDatabase.reset(new QFreeTypeFontDatabase());
#endif

#if QT_CONFIG(draganddrop)
    m_drag.reset(new QOffscreenDrag);
#endif
    m_services.reset(new QPlatformServices);

    QWindowSystemInterface::handleScreenAdded(new QOffscreenScreen);
}

QOffscreenIntegration::~QOffscreenIntegration()
{
}

void QOffscreenIntegration::initialize()
{
    m_inputContext.reset(QPlatformInputContextFactory::create());
}

QPlatformInputContext *QOffscreenIntegration::inputContext() const
{
    return m_inputContext.data();
}

bool QOffscreenIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps: return true;
    case MultipleWindows: return true;
    default: return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformWindow *QOffscreenIntegration::createPlatformWindow(QWindow *window) const
{
    Q_UNUSED(window);
    QPlatformWindow *w = new QOffscreenWindow(window);
    w->requestActivateWindow();
    return w;
}

QPlatformBackingStore *QOffscreenIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new QOffscreenBackingStore(window);
}

QAbstractEventDispatcher *QOffscreenIntegration::createEventDispatcher() const
{
#if defined(Q_OS_UNIX)
    return createUnixEventDispatcher();
#elif defined(Q_OS_WIN)
#ifndef Q_OS_WINRT
    return new QOffscreenEventDispatcher<QEventDispatcherWin32>();
#else // !Q_OS_WINRT
    return new QOffscreenEventDispatcher<QEventDispatcherWinRT>();
#endif // Q_OS_WINRT
#else
    return 0;
#endif
}

static QString themeName() { return QStringLiteral("offscreen"); }

QStringList QOffscreenIntegration::themeNames() const
{
    return QStringList(themeName());
}

// Restrict the styles to "fusion" to prevent native styles requiring native
// window handles (eg Windows Vista style) from being used.
class OffscreenTheme : public QPlatformTheme
{
public:
    OffscreenTheme() {}

    QVariant themeHint(ThemeHint h) const override
    {
        switch (h) {
        case StyleNames:
            return QVariant(QStringList(QStringLiteral("fusion")));
        default:
            break;
        }
        return QPlatformTheme::themeHint(h);
    }
};

QPlatformTheme *QOffscreenIntegration::createPlatformTheme(const QString &name) const
{
    return name == themeName() ? new OffscreenTheme() : nullptr;
}

QPlatformFontDatabase *QOffscreenIntegration::fontDatabase() const
{
    return m_fontDatabase.data();
}

#if QT_CONFIG(draganddrop)
QPlatformDrag *QOffscreenIntegration::drag() const
{
    return m_drag.data();
}
#endif

QPlatformServices *QOffscreenIntegration::services() const
{
    return m_services.data();
}

QOffscreenIntegration *QOffscreenIntegration::createOffscreenIntegration()
{
#if QT_CONFIG(xlib) && QT_CONFIG(opengl) && !QT_CONFIG(opengles2)
    QByteArray glx = qgetenv("QT_QPA_OFFSCREEN_NO_GLX");
    if (glx.isEmpty())
        return new QOffscreenX11Integration;
#endif
    return new QOffscreenIntegration;
}

QT_END_NAMESPACE
