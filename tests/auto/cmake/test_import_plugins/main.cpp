// Copyright (C) 2018 Kitware, Inc.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QList>
#include <QPluginLoader>
#include <QSet>
#include <QString>

#include <cstddef>
#include <iostream>

extern QString expectedPlugins[];
extern std::size_t numExpectedPlugins;

int main(int argc, char **argv)
{
#ifdef QT_STATIC
    QSet<QString> expectedPluginSet;
    for (std::size_t i = 0; i < numExpectedPlugins; i++) {
        expectedPluginSet.insert(expectedPlugins[i]);
    }

    QList<QStaticPlugin> plugins = QPluginLoader::staticPlugins();
    QSet<QString> actualPluginSet;
    for (QStaticPlugin plugin : plugins) {
        actualPluginSet.insert(plugin.metaData()["className"].toString());
    }

    if (expectedPluginSet != actualPluginSet) {
        std::cerr << "Loaded plugins do not match what was expected!" << std::endl
                  << "Expected plugins:" << std::endl;

        QList<QString> expectedPluginList = expectedPluginSet.values();
        expectedPluginList.sort();
        for (QString plugin : expectedPluginList) {
            std::cerr << (actualPluginSet.contains(plugin) ? "  " : "- ")
                      << plugin.toStdString() << std::endl;
        }

        std::cerr << std::endl << "Actual plugins:" << std::endl;

        QList<QString> actualPluginList = actualPluginSet.values();
        actualPluginList.sort();
        for (QString plugin : actualPluginList) {
            std::cerr << (expectedPluginSet.contains(plugin) ? "  " : "+ ")
                      << plugin.toStdString() << std::endl;
        }

        return 1;
    }

#endif
    return 0;
}
