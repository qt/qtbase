// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QTMODULEINFO_H
#define QTMODULEINFO_H

#include <QString>
#include <QStringList>

#include <bitset>
#include <vector>

constexpr size_t ModuleBitsetSize = 1024;
using ModuleBitset = std::bitset<ModuleBitsetSize>;

struct QtModule
{
    static constexpr size_t InvalidId = ModuleBitsetSize - 1;
    size_t id = InvalidId;
    bool internal = false;
    QString name;
    QString repository;
    QStringList pluginTypes;
    QString translationCatalog;
};

inline bool contains(const ModuleBitset &modules, const QtModule &module)
{
    return modules.test(module.id);
}

class QtModuleInfoStore
{
public:
    QtModuleInfoStore() = default;

    bool populate(const QString &modulesDir, const QString &translationsDir, bool verbose,
                  QString *errorString);

    size_t size() const { return modules.size(); }
    std::vector<QtModule>::const_iterator begin() const { return modules.begin(); }
    std::vector<QtModule>::const_iterator end() const { return modules.end(); }

    const QtModule &moduleById(size_t id) const;
    size_t moduleIdForPluginType(const QString &pluginType) const;

private:
    std::vector<QtModule> modules;
};

#endif
