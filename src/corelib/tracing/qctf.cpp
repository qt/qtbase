// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#define BUILD_LIBRARY

#include <qthread.h>
#include <qpluginloader.h>
#include <qfileinfo.h>
#include <qdir.h>
#include <qtenvironmentvariables.h>
#include <qjsonarray.h>

#include "qctf_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static bool s_initialized = false;
static bool s_triedLoading = false;
static bool s_prevent_recursion = false;
static bool s_shutdown = false;
static QCtfLib* s_plugin = nullptr;

#if QT_CONFIG(library) && defined(QT_SHARED)

#if defined(Q_OS_ANDROID)
static QString findPlugin(QLatin1StringView plugin)
{
    const QString pluginPath = qEnvironmentVariable("QT_PLUGIN_PATH");
    for (const auto &entry : QDirListing(pluginPath, QDirListing::IteratorFlag::FilesOnly)) {
        if (entry.fileName().contains(plugin))
            return entry.absoluteFilePath();
    }
    return {};
}
#endif

static bool loadPlugin(bool &retry)
{
    retry = false;
#ifdef Q_OS_WIN
#ifdef QT_DEBUG
    QPluginLoader loader(QStringLiteral("tracing/QCtfTracePlugind.dll"));
#else
    QPluginLoader loader(QStringLiteral("tracing/QCtfTracePlugin.dll"));
#endif
#elif defined(Q_OS_ANDROID)

    const QString plugin = findPlugin("QCtfTracePlugin"_L1);
    if (plugin.isEmpty()) {
        retry = true;
        return false;
    }
    QPluginLoader loader(plugin);
#else
    QPluginLoader loader(QStringLiteral("tracing/libQCtfTracePlugin.so"));
#endif

    if (!loader.isLoaded()) {
        if (!loader.load())
            return false;
    }
    s_plugin = qobject_cast<QCtfLib *>(loader.instance());
    if (!s_plugin)
        return false;
    s_plugin->shutdown(&s_shutdown);
    return true;
}

#else

#define QCtfPluginIID QStringLiteral("org.qt-project.Qt.QCtfLib")

static bool loadPlugin(bool &retry)
{
    retry = false;
    const auto &plugins = QPluginLoader::staticPlugins();
    for (const auto &plugin : plugins) {
        const auto json = plugin.metaData();
        const auto IID = json[QStringLiteral("IID")];
        if (IID.toString() == QCtfPluginIID) {
            s_plugin = qobject_cast<QCtfLib *>(plugin.instance());
            if (!s_plugin)
                return false;
            s_plugin->shutdown(&s_shutdown);
            return true;
        }
    }
    return false;
}

#endif

static bool initialize()
{
    if (s_shutdown || s_prevent_recursion)
        return false;
    if (s_initialized || s_triedLoading)
        return s_initialized;
    s_prevent_recursion = true;
    bool retry = false;
    if (!loadPlugin(retry)) {
        if (!retry) {
            s_triedLoading = true;
            s_initialized = false;
        }
    } else {
        bool enabled = s_plugin->sessionEnabled();
        if (!enabled) {
            s_triedLoading = true;
            s_initialized = false;
        } else {
            s_initialized = true;
        }
    }
    s_prevent_recursion = false;
    return s_initialized;
}

bool _tracepoint_enabled(const QCtfTracePointEvent &point)
{
    if (!initialize())
        return false;
    return s_plugin ? s_plugin->tracepointEnabled(point) : false;
}

void _do_tracepoint(const QCtfTracePointEvent &point, const QByteArray &arr)
{
    if (!initialize())
        return;
    if (s_plugin)
        s_plugin->doTracepoint(point, arr);
}

QCtfTracePointPrivate *_initialize_tracepoint(const QCtfTracePointEvent &point)
{
    if (!initialize())
        return nullptr;
    return s_plugin ? s_plugin->initializeTracepoint(point) : nullptr;
}

QT_END_NAMESPACE

#include "moc_qctf_p.cpp"
