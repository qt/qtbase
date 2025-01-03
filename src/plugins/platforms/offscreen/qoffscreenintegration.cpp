// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

using namespace Qt::StringLiterals;

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

QOffscreenIntegration::QOffscreenIntegration(const QStringList& paramList)
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

    QJsonObject config = resolveConfigFileConfiguration(paramList).value_or(defaultConfiguration());
    setConfiguration(config);
}

QOffscreenIntegration::~QOffscreenIntegration()
{
    while (!m_screens.isEmpty())
        QWindowSystemInterface::handleScreenRemoved(m_screens.takeLast());
}

/*
    The offscren platform plugin is configurable with a JSON configuration.
    The confiuration can be provided either from a file on disk on startup,
    or at by calling setConfiguration().

    To provide a configuration on startuip, write the config to disk and pass
    the file path as a platform argument:

        ./myapp -platform offscreen:configfile=/path/to/config.json

    The supported top-level config keys are:
    {
        "synchronousWindowSystemEvents": <bool>
        "windowFrameMargins": <bool>,
        "screens": [<screens>],
    }

    "screens" is an array of:
    {
        "name": string,
        "x": int,
        "y": int,
        "width": int,
        "height": int,
        "logicalDpi": int,
        "logicalBaseDpi": int,
        "dpr": double,
    }
*/

QJsonObject QOffscreenIntegration::defaultConfiguration() const
{
    const auto defaultScreen = QJsonObject {
        {"name", ""},
        {"x", 0},
        {"y", 0},
        {"width", 800},
        {"height", 800},
        {"logicalDpi", 96},
        {"logicalBaseDpi", 96},
        {"dpr", 1.0},
    };
    const auto defaultConfiguration = QJsonObject {
        {"synchronousWindowSystemEvents", false},
        {"windowFrameMargins", true},
        {"screens", QJsonArray { defaultScreen } },
    };
    return defaultConfiguration;
}

std::optional<QJsonObject> QOffscreenIntegration::resolveConfigFileConfiguration(const QStringList& paramList) const
{
    bool hasConfigFile = false;
    QString configFilePath;
    for (const QString &param : paramList) {
        // Look for "configfile=/path/to/file/"
        QString configPrefix("configfile="_L1);
        if (param.startsWith(configPrefix)) {
            hasConfigFile = true;
            configFilePath = param.mid(configPrefix.size());
        }
    }
    if (!hasConfigFile)
        return std::nullopt;

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

    return config.object();
}


void QOffscreenIntegration::setConfiguration(const QJsonObject &configuration)
{
    // Apply the new configuration, diffing against the current m_configuration

    const bool synchronousWindowSystemEvents = configuration["synchronousWindowSystemEvents"].toBool(
        m_configuration["synchronousWindowSystemEvents"].toBool(false));
    QWindowSystemInterface::setSynchronousWindowSystemEvents(synchronousWindowSystemEvents);

    m_windowFrameMarginsEnabled = configuration["windowFrameMargins"].toBool(
                m_configuration["windowFrameMargins"].toBool(true));

    // Diff screens array, using the screen name as the screen identity.
    QJsonArray currentScreens = m_configuration["screens"].toArray();
    QJsonArray newScreens = configuration["screens"].toArray();

    auto getScreenNames = [](const QJsonArray &screens) -> QList<QString> {
        QList<QString> names;
        for (QJsonValue screen : screens) {
            names.append(screen["name"].toString());
        };
        std::sort(names.begin(), names.end());
        return names;
    };

    auto currentNames = getScreenNames(currentScreens);
    auto newNames = getScreenNames(newScreens);

    QList<QString> present;
    std::set_intersection(currentNames.begin(), currentNames.end(), newNames.begin(), newNames.end(),
                          std::inserter(present, present.begin()));
    QList<QString> added;
    std::set_difference(newNames.begin(), newNames.end(), currentNames.begin(), currentNames.end(),
                          std::inserter(added, added.begin()));
    QList<QString> removed;
    std::set_difference(currentNames.begin(), currentNames.end(), newNames.begin(), newNames.end(),
                              std::inserter(removed, removed.begin()));

    auto platformScreenByName = [](const QString &name, QList<QOffscreenScreen *> screens) -> QOffscreenScreen * {
        for (QOffscreenScreen *screen : screens) {
            if (screen->m_name == name)
                return screen;
        }
        Q_UNREACHABLE();
    };

    auto screenConfigByName = [](const QString &name, QJsonArray screenConfigs) -> QJsonValue {
        for (QJsonValue screenConfig : screenConfigs) {
            if (screenConfig["name"].toString() == name)
                return screenConfig;
        }
        Q_UNREACHABLE();
    };

    auto geometryFromConfig = [](const QJsonObject &config) -> QRect {
        return QRect(config["x"].toInt(0), config["y"].toInt(0), config["width"].toInt(640), config["height"].toInt(480));
    };

    // Remove removed screens
    for (const QString &remove : removed) {
        QOffscreenScreen *screen = platformScreenByName(remove, m_screens);
        m_screens.removeAll(screen);
        QWindowSystemInterface::handleScreenRemoved(screen);
    }

    // Add new screens
    for (const QString &add : added) {
        QJsonValue configValue = screenConfigByName(add, newScreens);
        QJsonObject config  = configValue.toObject();
        if (config.isEmpty()) {
            qWarning("empty screen object");
            continue;
        }
        QOffscreenScreen *offscreenScreen = new QOffscreenScreen(this);
        offscreenScreen->m_name = config["name"].toString();
        offscreenScreen->m_geometry = geometryFromConfig(config);
        offscreenScreen->m_logicalDpi = config["logicalDpi"].toInt(96);
        offscreenScreen->m_logicalBaseDpi = config["logicalBaseDpi"].toInt(96);
        offscreenScreen->m_dpr = config["dpr"].toDouble(1.0);
        m_screens.append(offscreenScreen);
        QWindowSystemInterface::handleScreenAdded(offscreenScreen);
    }

    // Update present screens
    for (const QString &pres : present) {
        QOffscreenScreen *screen = platformScreenByName(pres, m_screens);
        Q_ASSERT(screen);
        QJsonObject currentConfig = screenConfigByName(pres, currentScreens).toObject();
        QJsonObject newConfig = screenConfigByName(pres, newScreens).toObject();

        // Name can't change, because it'd be a different screen
        Q_ASSERT(currentConfig["name"] == newConfig["name"]);

        // Geometry
        QRect currentGeomtry = geometryFromConfig(currentConfig);
        QRect newGeomtry = geometryFromConfig(newConfig);
        if (currentGeomtry != newGeomtry) {
            screen->m_geometry = newGeomtry;
            QWindowSystemInterface::handleScreenGeometryChange(screen->screen(), newGeomtry, newGeomtry);
        }

        // logical DPI
        int currentLogicalDpi = currentConfig["logicalDpi"].toInt(96);
        int newLogicalDpi = newConfig["logicalDpi"].toInt(96);
        if (currentLogicalDpi != newLogicalDpi) {
            screen->m_logicalDpi = newLogicalDpi;
            QWindowSystemInterface::handleScreenLogicalDotsPerInchChange(screen->screen(), newLogicalDpi, newLogicalDpi);
        }

        // The base DPI is more of a platform constant, and should not change, and
        // there is no handleChange function for it. Print a warning.
        int currentLogicalBaseDpi = currentConfig["logicalBaseDpi"].toInt(96);
        int newLogicalBaseDpi = newConfig["logicalBaseDpi"].toInt(96);
        if (currentLogicalBaseDpi != newLogicalBaseDpi) {
            screen->m_logicalBaseDpi = newLogicalBaseDpi;
            qWarning("You ain't supposed to change logicalBaseDpi - its a platform constant. Qt may not react to the change");
        }

        // DPR. There is also no handleChange function in Qt at this point, instead
        // the new DPR value will be used during the next repaint. We could repaint
        // all windows here, but don't. Print a warning.
        double currentDpr = currentConfig["dpr"].toDouble(1);
        double newDpr = newConfig["dpr"].toDouble(1);
        if (currentDpr != newDpr) {
            screen->m_dpr = newDpr;
            qWarning("DPR change notifications is not implemented - Qt may not react to the change");
        }
    }

    // Now the new configuration is the current configuration
    m_configuration = configuration;
}

QJsonObject QOffscreenIntegration::configuration() const
{
    return m_configuration;
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
        m_nativeInterface.reset(new QOffscreenPlatformNativeInterface(const_cast<QOffscreenIntegration*>(this)));
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
        static QFont systemFont("Sans Serif"_L1, 9);
        static QFont fixedFont("monospace"_L1, 9);
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
        offscreenIntegration = new QOffscreenX11Integration(paramList);
#endif

     if (!offscreenIntegration)
        offscreenIntegration = new QOffscreenIntegration(paramList);
    return offscreenIntegration;
}

QList<QOffscreenScreen *> QOffscreenIntegration::screens() const
{
    return m_screens;
}

QT_END_NAMESPACE
