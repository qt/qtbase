// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QTPLUGININFO_H
#define QTPLUGININFO_H

#include "utils.h"

#include <QString>
#include <QStringList>

#include <unordered_map>

enum class PluginDetection
{
    DebugOnly,
    ReleaseOnly,
    DebugAndRelease
};

struct PluginLists
{
    QStringList disabledPluginTypes;
    QStringList enabledPluginTypes;
    QStringList excludedPlugins;
    QStringList includedPlugins;
};

class PluginInformation
{
public:
    PluginInformation() = default;

    bool isTypeForPlugin(const QString &type, const QString &plugin) const;

    void generateAvailablePlugins(const QMap<QString, QString> &qtPathsVariables,
                                  const Platform &platform);
    void populatePluginToType(const QDir &pluginDir, const QStringList &plugins);

    const std::unordered_map<QString, QStringList> &typeMap() const { return m_typeMap; }

private:
    std::unordered_map<QString, QStringList> m_typeMap;
    std::unordered_map<QString, QString> m_pluginMap;
};

#endif
