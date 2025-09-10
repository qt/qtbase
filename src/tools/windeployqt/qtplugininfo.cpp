// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qtplugininfo.h"

#include <QDir>

static PluginDetection determinePluginLibrary(const QDir &platformPluginDir, const QString &infix)
{
    // Use the platform plugin to determine which dlls are there (release/debug/both)
    QString platformReleaseFilter(QStringLiteral("qwindows"));
    if (!infix.isEmpty())
        platformReleaseFilter += infix;
    QString platformFilter = platformReleaseFilter + u'*';
    platformFilter += sharedLibrarySuffix();

    const QFileInfoList &dlls =
            platformPluginDir.entryInfoList(QStringList(platformFilter), QDir::Files);
    if (dlls.size() == 1) {
        const QFileInfo dllFi = dlls.first();
        const bool hasDebugDlls =
                dllFi.fileName() == QString(platformReleaseFilter + sharedLibrarySuffix()) ? false
                                                                                           : true;
        return (hasDebugDlls ? PluginDetection::DebugOnly : PluginDetection::ReleaseOnly);
    } else {
        return PluginDetection::DebugAndRelease;
    }
}

static QStringList findPluginNames(const QDir &pluginDir, const PluginDetection libraryType,
                                   const Platform &platform)
{
    QString errorMessage{};
    QStringList result{};
    QString filter{};
    filter += u"*";
    filter += sharedLibrarySuffix();

    const QFileInfoList &dlls =
            pluginDir.entryInfoList(QStringList(filter), QDir::Files, QDir::Name);

    for (const QFileInfo &dllFi : dlls) {
        QString plugin = dllFi.fileName();
        const int dotIndex = plugin.lastIndexOf(u'.');
        // We don't need the .dll for the name
        plugin = plugin.first(dotIndex);

        if (libraryType == PluginDetection::DebugAndRelease) {
            bool isDebugDll{};
            if (!readPeExecutable(dllFi.absoluteFilePath(), &errorMessage, 0, 0, &isDebugDll,
                                  (platform == WindowsDesktopMinGW))) {
                std::wcerr << "Warning: Unable to read "
                           << QDir::toNativeSeparators(dllFi.absoluteFilePath()) << ": "
                           << errorMessage;
            }
            if (isDebugDll && platformHasDebugSuffix(platform))
                plugin.removeLast();
        }
        else if (libraryType == PluginDetection::DebugOnly)
            plugin.removeLast();

        if (!result.contains(plugin))
            result.append(plugin);
    }
    return result;
}

bool PluginInformation::isTypeForPlugin(const QString &type, const QString &plugin) const
{
    return m_pluginMap.at(plugin) == type;
}

void PluginInformation::populatePluginToType(const QDir &pluginDir, const QStringList &plugins)
{
    for (const QString &plugin : plugins)
        m_pluginMap.insert({ plugin, pluginDir.dirName() });
}

void PluginInformation::generateAvailablePlugins(const QMap<QString, QString> &qtPathsVariables,
                                                 const Platform &platform)
{
    const QDir pluginTypesDir(qtPathsVariables.value(QLatin1String("QT_INSTALL_PLUGINS")));
    const QDir platformPluginDir(pluginTypesDir.absolutePath() + QStringLiteral("/platforms"));
    const QString infix(qtPathsVariables.value(QLatin1String(qmakeInfixKey)));
    const PluginDetection debugDetection = determinePluginLibrary(platformPluginDir, infix);

    const QFileInfoList &pluginTypes =
            pluginTypesDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &pluginType : pluginTypes) {
        const QString pluginTypeName = pluginType.baseName();
        m_typeMap.insert({ pluginTypeName, QStringList{} });
        const QStringList plugins =
                findPluginNames(pluginType.absoluteFilePath(), debugDetection, platform);
        m_typeMap.at(pluginTypeName) = plugins;
        populatePluginToType(pluginTypeName, plugins);
    }
    if (!m_typeMap.size() || !m_pluginMap.size())
        std::wcerr << "Warning: could not parse available plugins properly, plugin "
                      "inclusion/exclusion options will not work\n";
}
