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
#include <QtGui/private/qgenericunixeventdispatcher_p.h>
#if defined(Q_OS_MAC)
#include <qpa/qplatformfontdatabase.h>
#include <QtGui/private/qcoretextfontdatabase_p.h>
#else
#include <QtGui/private/qgenericunixfontdatabase_p.h>
#endif
#elif defined(Q_OS_WIN)
#include <QtGui/private/qfreetypefontdatabase_p.h>
#include <QtCore/private/qeventdispatcher_win_p.h>
#endif

#include <QtCore/qfile.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qjsonvalue.h>
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

    bool processEvents(QEventLoop::ProcessEventsFlags flags) override
    {
        bool didSendEvents = BaseEventDispatcher::processEvents(flags);

        return QWindowSystemInterface::sendWindowSystemEvents(flags) || didSendEvents;
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
}

QOffscreenIntegration::~QOffscreenIntegration()
{
    for (auto screen : std::as_const(m_screens))
        QWindowSystemInterface::handleScreenRemoved(screen);
}

/*
    The offscren platform plugin is configurable with a JSON configuration
    file. Write the config to disk and pass the file path as a platform argument:

        ./myapp -platform offscreen:configfile=/path/to/config.json

    The supported top-level config keys are:
    {
        "synchronousWindowSystemEvents": <bool>
        "windowFrameMargins": <bool>,
        "screens": [<screens>],
    }

    Screen:
    {
        "name" : string,
        "x": int,
        "y": int,
        "width": int,
        "height": int,
        "logicalDpi": int,
        "logicalBaseDpi": int,
        "dpr": double,
    }
*/
void QOffscreenIntegration::configure(const QStringList& paramList)
{
    // Use config file configuring platform plugin, if one was specified
    bool hasConfigFile = false;
    QString configFilePath;
    for (const QString &param : paramList) {
        // Look for "configfile=/path/to/file/"
        QString configPrefix(QLatin1String("configfile="));
        if (param.startsWith(configPrefix)) {
            hasConfigFile = true;
            configFilePath= param.mid(configPrefix.length());
        }
    }

    // Create the default screen if there was no config file
    if (!hasConfigFile) {
        QOffscreenScreen *offscreenScreen = new QOffscreenScreen(this);
        m_screens.append(offscreenScreen);
        QWindowSystemInterface::handleScreenAdded(offscreenScreen);
        return;
    }

    // Read config file
    if (configFilePath.isEmpty())
        qFatal("Missing file path for -configfile platform option");
    QFile configFile(configFilePath);
    if (!configFile.exists())
        qFatal("Could not find platform config file %s", qPrintable(configFilePath));
    if (!configFile.open(QIODevice::ReadOnly))
        qFatal("Could not open platform config file for reading %s, %s", qPrintable(configFilePath), qPrintable(configFile.errorString()));

    QByteArray json = configFile.readAll();
    QJsonParseError error;
    QJsonDocument config = QJsonDocument::fromJson(json, &error);
    if (config.isNull())
        qFatal("Platform config file parse error: %s", qPrintable(error.errorString()));

    // Apply configuration (create screens)
    bool synchronousWindowSystemEvents = config["synchronousWindowSystemEvents"].toBool(false);
    QWindowSystemInterface::setSynchronousWindowSystemEvents(synchronousWindowSystemEvents);
    m_windowFrameMarginsEnabled = config["windowFrameMargins"].toBool(true);
    QJsonArray screens = config["screens"].toArray();
    for (QJsonValue screenValue : screens) {
        QJsonObject screen  = screenValue.toObject();
        if (screen.isEmpty()) {
            qWarning("QOffscreenIntegration::initializeWithPlatformArguments: empty screen object");
            continue;
        }
        QOffscreenScreen *offscreenScreen = new QOffscreenScreen(this);
        offscreenScreen->m_name = screen["name"].toString();
        offscreenScreen->m_geometry = QRect(screen["x"].toInt(0), screen["y"].toInt(0),
                                            screen["width"].toInt(640), screen["height"].toInt(480));
        offscreenScreen->m_logicalDpi = screen["logicalDpi"].toInt(96);
        offscreenScreen->m_logicalBaseDpi = screen["logicalBaseDpi"].toInt(96);
        offscreenScreen->m_dpr = screen["dpr"].toDouble(1.0);

        m_screens.append(offscreenScreen);
        QWindowSystemInterface::handleScreenAdded(offscreenScreen);
    }
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
    case RhiBasedRendering: return false;
    default: return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformWindow *QOffscreenIntegration::createPlatformWindow(QWindow *window) const
{
    Q_UNUSED(window);
    QPlatformWindow *w = new QOffscreenWindow(window, m_windowFrameMarginsEnabled);
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
    return new QOffscreenEventDispatcher<QEventDispatcherWin32>();
#else
    return 0;
#endif
}

QPlatformNativeInterface *QOffscreenIntegration::nativeInterface() const
{
    if (!m_nativeInterface)
        m_nativeInterface.reset(new QOffscreenPlatformNativeInterface);
    return m_nativeInterface.get();
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
            return QVariant(QStringList(QStringLiteral("Fusion")));
        default:
            break;
        }
        return QPlatformTheme::themeHint(h);
    }

    virtual const QFont *font(Font type = SystemFont) const override
    {
        static QFont systemFont(QLatin1String("Sans Serif"), 9);
        static QFont fixedFont(QLatin1String("monospace"), 9);
        switch (type) {
        case QPlatformTheme::SystemFont:
            return &systemFont;
        case QPlatformTheme::FixedFont:
            return &fixedFont;
        default:
            return nullptr;
        }
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

QOffscreenIntegration *QOffscreenIntegration::createOffscreenIntegration(const QStringList& paramList)
{
    QOffscreenIntegration *offscreenIntegration = nullptr;

#if QT_CONFIG(xlib) && QT_CONFIG(opengl) && !QT_CONFIG(opengles2)
    QByteArray glx = qgetenv("QT_QPA_OFFSCREEN_NO_GLX");
    if (glx.isEmpty())
        offscreenIntegration = new QOffscreenX11Integration;
#endif

     if (!offscreenIntegration)
        offscreenIntegration = new QOffscreenIntegration;

    offscreenIntegration->configure(paramList);
    return offscreenIntegration;
}

QList<QPlatformScreen *> QOffscreenIntegration::screens() const
{
    return m_screens;
}

QT_END_NAMESPACE
