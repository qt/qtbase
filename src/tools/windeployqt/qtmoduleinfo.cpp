// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qtmoduleinfo.h"
#include "utils.h"

#include <QDirListing>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

#include <iostream>
#include <algorithm>
#include <unordered_map>

using namespace Qt::StringLiterals;

static QStringList toStringList(const QJsonArray &jsonArray)
{
    QStringList result;
    for (const auto &item : jsonArray) {
        if (item.isString())
            result.append(item.toString());
    }
    return result;
}

struct TranslationCatalog
{
    QString name;
    QStringList repositories;
    QStringList modules;
};

using TranslationCatalogs = std::vector<TranslationCatalog>;

static TranslationCatalogs readTranslationsCatalogs(const QString &translationsDir,
                                                    bool verbose,
                                                    QString *errorString)
{
    QFile file(translationsDir + QLatin1String("/catalogs.json"));
    if (verbose) {
        std::wcerr << "Trying to read translation catalogs from \""
                   << qUtf8Printable(file.fileName()) << "\".\n";
    }
    if (!file.open(QIODevice::ReadOnly)) {
        *errorString = QLatin1String("Cannot open ") + file.fileName();
        return {};
    }

    QJsonParseError jsonParseError;
    QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &jsonParseError);
    if (jsonParseError.error != QJsonParseError::NoError) {
        *errorString = jsonParseError.errorString();
        return {};
    }

    if (!document.isArray()) {
        *errorString = QLatin1String("Expected an array as root element of ") + file.fileName();
        return {};
    }

    TranslationCatalogs catalogs;
    for (const QJsonValueRef &item : document.array()) {
        TranslationCatalog catalog;
        catalog.name = item[QLatin1String("name")].toString();
        catalog.repositories = toStringList(item[QLatin1String("repositories")].toArray());
        catalog.modules = toStringList(item[QLatin1String("modules")].toArray());
        if (verbose)
            std::wcerr << "Found catalog \"" << qUtf8Printable(catalog.name) << "\".\n";
        catalogs.emplace_back(std::move(catalog));
    }

    return catalogs;
}

static QtModule moduleFromJsonFile(const QString &filePath, QString *errorString)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        *errorString = QLatin1String("Cannot open ") + file.fileName();
        return {};
    }

    QJsonParseError jsonParseError;
    QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &jsonParseError);
    if (jsonParseError.error != QJsonParseError::NoError) {
        *errorString = jsonParseError.errorString();
        return {};
    }

    if (!document.isObject()) {
        *errorString = QLatin1String("Expected an object as root element of ") + file.fileName();
        return {};
    }

    const QJsonObject obj = document.object();
    QtModule module;
    module.name = "Qt6"_L1 + obj[QLatin1String("name")].toString();
    module.repository = obj[QLatin1String("repository")].toString();
    module.internal = obj[QLatin1String("internal")].toBool();
    module.pluginTypes = toStringList(obj[QLatin1String("plugin_types")].toArray());
    return module;
}

static void dump(const QtModule &module)
{
    std::wcerr << "Found module \"" << qUtf8Printable(module.name) << "\".\n";
    if (!module.pluginTypes.isEmpty())
        qDebug().nospace() << "  plugin types: " << module.pluginTypes;
    if (!module.translationCatalog.isEmpty())
        qDebug().nospace() << "  translation catalog: "<< module.translationCatalog;
}

bool QtModuleInfoStore::populate(const QString &modulesDir, const QString &translationsDir,
                                 bool verbose, QString *errorString)
{
    const TranslationCatalogs catalogs = readTranslationsCatalogs(translationsDir, verbose,
                                                                  errorString);
    if (!errorString->isEmpty()) {
        std::wcerr << "Warning: Translations will not be available due to the following error."
                   << std::endl << *errorString << std::endl;
        errorString->clear();
    }
    std::unordered_map<QString, QString> moduleToCatalogMap;
    std::unordered_map<QString, QString> repositoryToCatalogMap;
    for (const TranslationCatalog &catalog : catalogs) {
        for (const QString &module : catalog.modules) {
            moduleToCatalogMap.insert(std::make_pair(module, catalog.name));
        }
        for (const QString &repository : catalog.repositories) {
            repositoryToCatalogMap.insert(std::make_pair(repository, catalog.name));
        }
    }

    using F = QDirListing::IteratorFlag;
    // Read modules, and assign a bit as ID.
    for (const auto &dirEntry : QDirListing(modulesDir, {u"*.json"_s}, F::FilesOnly)) {
        QtModule module = moduleFromJsonFile(dirEntry.filePath(), errorString);
        if (!errorString->isEmpty())
            return false;
        if (module.internal && module.name.endsWith(QStringLiteral("Private")))
            module.name.chop(7);
        module.id = modules.size();
        if (module.id == QtModule::InvalidId) {
            *errorString = "Internal Error: too many modules for ModuleBitset to hold."_L1;
            return false;
        }

        {
            auto it = moduleToCatalogMap.find(module.name);
            if (it != moduleToCatalogMap.end())
                module.translationCatalog = it->second;
        }
        if (module.translationCatalog.isEmpty()) {
            auto it = repositoryToCatalogMap.find(module.repository);
            if (it != repositoryToCatalogMap.end())
                module.translationCatalog = it->second;
        }
        if (verbose)
            dump(module);
        modules.emplace_back(std::move(module));
    }

    return true;
}

const QtModule &QtModuleInfoStore::moduleById(size_t id) const
{
    return modules.at(id);
}

size_t QtModuleInfoStore::moduleIdForPluginType(const QString &pluginType) const
{
    auto moduleHasPluginType = [&pluginType] (const QtModule &module) {
        return module.pluginTypes.contains(pluginType);
    };

    auto it = std::find_if(modules.begin(), modules.end(), moduleHasPluginType);
    if (it != modules.end())
        return it->id ;

    return QtModule::InvalidId;
}
