// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#define BUILD_LIBRARY

#include <qthread.h>
#include <qpluginloader.h>
#include <qfileinfo.h>
#include <qdir.h>

#include "qctf_p.h"

QT_BEGIN_NAMESPACE

static bool s_initialized = false;
static bool s_triedLoading = false;
static bool s_prevent_recursion = false;
static bool s_shutdown = false;
static QCtfLib* s_plugin = nullptr;

#if defined(Q_OS_ANDROID)
static QString findPlugin(const QString &plugin)
{
    QString pluginPath = QString::fromUtf8(qgetenv("QT_PLUGIN_PATH"));
    QDir dir(pluginPath);
    const QStringList files = dir.entryList(QDir::Files);
    for (const QString &file : files) {
        if (file.contains(plugin))
            return QFileInfo(pluginPath + QLatin1Char('/') + file).absoluteFilePath();
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

    QString plugin = findPlugin(QStringLiteral("QCtfTracePlugin"));
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
